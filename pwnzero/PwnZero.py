import logging
import serial
import time
from enum import Enum

import pwnagotchi
import pwnagotchi.plugins as plugins
import pwnagotchi.ui.faces as faces


class Packet(Enum):
    """
    These are control bytes and mark the beginning and end of a packet
    These values are reserved, and cannot be used as commands or in the body of a packet
    """
    START   = 0x02
    END     = 0x03

class FlipperCommand(Enum):
    """
    Flipper Zero Commands
    These commands can be sent from the pwnagotchi to the flipper
    """
    # Control Values
    SYN   = 0x16
    ACK   = 0x06
    NAK   = 0x15

    # Flipper parameters
    UI_FACE        = 0x04
    UI_NAME        = 0x05
    UI_APS         = 0x07
    UI_UPTIME      = 0x08
    UI_FRIEND      = 0x09
    UI_MODE        = 0x0A
    UI_HANDSHAKES  = 0x0B
    UI_STATUS      = 0x0C
    UI_CHANNEL     = 0x0D


class PwnCommand(Enum):
    """
    Pwnagotchi commands
    These commands can be sent from the Flipper to the pwnagotchi
    """
    # Control Values
    SYN   = 0x16
    ACK   = 0x06
    NAK   = 0x15

    # pwnagotchi commands
    REBOOT         = 0x04
    SHUTDOWN       = 0x05
    MODE           = 0x07
    UI_REFRESH     = 0x08 # request a ui refresh from the pwnagotchi
    CLOCK_SET      = 0x09 # flipper has a hardware clock, pwnagotchi does not. lets leverage that

    #TODO add ability to send commands to bettercap

class PwnMode(Enum):
    """
    Embedded class with the mode
    """
    MANU    = 0x04
    AUTO    = 0x05
    AI      = 0x06

class PwnFace(Enum):
    """
    Embedded class with all face parameters
    """
    NO_FACE         = 0x04
    DEFAULT_FACE    = 0x05
    LOOK_R          = 0x06
    LOOK_L          = 0x07
    LOOK_R_HAPPY    = 0x08
    LOOK_L_HAPPY    = 0x09
    SLEEP           = 0x0A
    SLEEP2          = 0x0B
    AWAKE           = 0x0C
    BORED           = 0x0D
    INTENSE         = 0x0E
    COOL            = 0x0F
    HAPPY           = 0x10
    GRATEFUL        = 0x11
    EXCITED         = 0x12
    MOTIVATED       = 0x13
    DEMOTIVATED     = 0x14
    SMART           = 0x15
    LONELY          = 0x16
    SAD             = 0x17
    ANGRY           = 0x18
    FRIEND          = 0x19
    BROKEN          = 0x1A
    DEBUG           = 0x1B
    UPLOAD          = 0x1C
    UPLOAD1         = 0x1D
    UPLOAD2         = 0x1E

class SerialConnException(Exception):
    pass

class ReceivedNone(Exception):
    pass

class PwnZeroSerialException(Exception):
    pass


class SendLengthIncorrect(PwnZeroSerialException):
    pass

class ReceivedMalformedStart(PwnZeroSerialException):
    pass

class ReceivedMalformedEnd(PwnZeroSerialException):
    pass

class ReceivedNak(PwnZeroSerialException):
    pass

class ReceivedInvalidAck(PwnZeroSerialException):
    pass

# helper functions
def _str_to_bytes(s: str):
    """
    Converts a string into a list of ascii encoded bytes

    :param: s: String to convert
    :return: List of bytes
    """
    retVal = []
    for c in s:
        retVal.append(ord(c))

    return retVal

def _ui_diff(current_ui, new_ui, key):
    if current_ui is not None:
        if current_ui.get(key) == new_ui.get(key):
            return False

    return True


class Flipper():

    def __init__(self, port: str = "/dev/serial0", baud: int = 115200, timeout: float = 1):
        """
        Construct a Flipper object, this will create the connection to the flipper

        :param: port: Port on which the UART of the Flipper is connected to
        :param: baud: Baudrate for communication to the Flipper (default 115200)
        """

        # rather than have to keep a list of all of our ui setters, generate one
        self._ui_setters = [method for method in dir(self) if callable(getattr(self, method)) if method.startswith('set_')]

        self._port = port
        self._baud = baud
        self._timeout = timeout

        self._serial_conn = None


    def _send_string(self, cmd: int, msg_string: str) -> bool:
        ascii_encoded_string = _str_to_bytes(msg_string)
        self._send_bytes(cmd, ascii_encoded_string)

    def _send_bytes(self, cmd: int, body: [int]) -> bool:
        """
        Sends a packet using protocol v3 over the serial port to the Flipper Zero

        :param: cmd: Parameter that is being changed
        :param: body: Arguments to pass to the flipper. Must be a list of bytes
        :return: If transmission was successful
        """
        # Build the packet to send
        packet = [Packet.START.value, cmd] + body + [Packet.END.value]

        # Send data to flipper
        logging.info(f"[PwnZero] sending bytes {packet}")
        if not self._serial_conn.write(packet) == len(packet):
            raise SendLengthIncorrect()

        # if we are sending, we don't expect an ACK back
        if cmd == FlipperCommand.ACK.value or cmd == FlipperCommand.NAK.value:
            return

        # expect an ACK reply
        rec = self.receive_bytes()
        if rec != [PwnCommand.ACK.value]:
            logging.error(f"[PwnZero] received non-ack response to syn: {rec}, expected: {[FlipperCommand.ACK.value]}")
            raise ReceivedInvalidAck(f"Expceted {[FlipperCommand.ACK.value]}, received: {rec}")

    def receive_bytes(self):

        def cleanup_and_raise(e):
            self._serial_conn.reset_input_buffer()
            raise e

        try:
            packet = self._serial_conn.read_until(expected=str(Packet.END.value))
        except Exception as e:
            logging.error(f"[PwnZero] failed reading from serial {e}")
            cleanup_and_raise(SerialConnException(e))

        if len(packet) < 1:
            # nothing to clean up, so just raise
            raise ReceivedNone()

        # convert from byte string in the form  b'\x02\x06\x03'
        # to a list of bytes [2, 6, 3]
        packet = list(packet)

        # if Packet.END.value is not in the line, we received something but timed out before seeing the end marker
        if packet[-1] != Packet.END.value:
            logging.error(f"[PwnZero] timed out reading Packet.END.value. received: {packet}")
            cleanup_and_raise(ReceivedMalformedEnd(f"timed out reading Packet.END.value. received: {packet}"))

        if packet[0] != Packet.START.value:
            logging.error(f"[PwnZero] packet did not begin with Packet.START.value. received: {packet}")
            cleanup_and_raise(ReceivedMalformedStart(f"packet did not begin with Packet.START.value. received: {packet}"))

        logging.info(f"[PwnZero] received packet: {packet}")
        # strip the Packet control characters
        body = packet[1:-1]

        if body[0] == PwnCommand.NAK.value:
            logging.error(f"[PwnZero] received NAK: {body}")
            cleanup_and_raise(ReceivedNak(f"NAK: {body}"))

        return body

    def open_serial(self):
        logging.info(f"[PwnZero] opening serial connection")
        try:
            #TODO investigate using rtscts=1
            # on flipper, take a look at "usb_uart_update_ctrl_lines"
            # for the rpi, likely need a device overlay https://forums.raspberrypi.com/viewtopic.php?t=241623
            # only really necessary if we start missing bytes
            # self._serial_conn = serial.Serial(self._port, self._baud, self._timeout)
            self._serial_conn = serial.Serial(self._port, self._baud, timeout=self._timeout)
            self._serial_conn.reset_input_buffer()
        except:
            raise "Cannot bind to port ({}) with baud ({})".format(self._port, self._baud)
        logging.info(f"[PwnZero] opened serial connection")

    def close_serial(self):
        # clean up the serial connection
        self._serial_conn.close()

    def send_syn(self):
        """
        Sends a syn packet to the flipper

        :return: If syn ack was successful
        """
        self._send_bytes(FlipperCommand.SYN.value, [])


    def send_ack(self):
        """
        Sends a ack packet to the flipper
        """
        self._send_bytes(FlipperCommand.ACK.value, [])


    def send_nak(self):
        """
        Sends a nak packet to the flipper
        """
        self._send_bytes(FlipperCommand.NAK.value, [])

    def update_ui(self, current_ui, new_ui) -> bool:
        """
        Set the ui elements of the Pwnagotchi
        Calls all Flipper methods that start with "set_"

        :return: If any ui setter fails, log the failure and return False
        """
        for method in self._ui_setters:
            try:
                ret = getattr(self, method)(current_ui, new_ui)  # call
            except Exception as e:
                logging.error(f"[PwnZero] error when calling ui setter {method}: {type(e).__name__}:{e.args}")

    def set_face(self, current_ui, new_ui) -> bool:
        """
        Set the face of the Pwnagotchi

        :return: If the command was sent successfully
        """

        face = new_ui.get('face')
        if not _ui_diff(current_ui=current_ui, new_ui=new_ui, key='face'):
            return True

        logging.info(f"[PwnZero] ui differs, setting face")

        faceEnum = None
        if face == faces.LOOK_R:
            faceEnum = PwnFace.LOOK_R
        elif face == faces.LOOK_L:
            faceEnum = PwnFace.LOOK_L
        elif face == faces.LOOK_R_HAPPY:
            faceEnum = PwnFace.LOOK_R_HAPPY
        elif face == faces.LOOK_L_HAPPY:
            faceEnum = PwnFace.LOOK_L_HAPPY
        elif face == faces.SLEEP:
            faceEnum = PwnFace.SLEEP
        elif face == faces.SLEEP2:
            faceEnum = PwnFace.SLEEP2
        elif face == faces.AWAKE:
            faceEnum = PwnFace.AWAKE
        elif face == faces.BORED:
            faceEnum = PwnFace.BORED
        elif face == faces.INTENSE:
            faceEnum = PwnFace.INTENSE
        elif face == faces.COOL:
            faceEnum = PwnFace.COOL
        elif face == faces.HAPPY:
            faceEnum = PwnFace.HAPPY
        elif face == faces.GRATEFUL:
            faceEnum = PwnFace.GRATEFUL
        elif face == faces.EXCITED:
            faceEnum = PwnFace.EXCITED
        elif face == faces.MOTIVATED:
            faceEnum = PwnFace.MOTIVATED
        elif face == faces.DEMOTIVATED:
            faceEnum = PwnFace.DEMOTIVATED
        elif face == faces.SMART:
            faceEnum = PwnFace.SMART
        elif face == faces.LONELY:
            faceEnum = PwnFace.LONELY
        elif face == faces.SAD:
            faceEnum = PwnFace.SAD
        elif face == faces.ANGRY:
            faceEnum = PwnFace.ANGRY
        elif face == faces.FRIEND:
            faceEnum = PwnFace.FRIEND
        elif face == faces.BROKEN:
            faceEnum = PwnFace.BROKEN
        elif face == faces.DEBUG:
            faceEnum = PwnFace.DEBUG
        elif face == faces.UPLOAD:
            faceEnum = PwnFace.UPLOAD
        elif face == faces.UPLOAD1:
            faceEnum = PwnFace.UPLOAD1
        elif face == faces.UPLOAD2:
            faceEnum = PwnFace.UPLOAD2

        return self._send_bytes(FlipperCommand.UI_FACE.value, [faceEnum.value])

    def set_name(self, current_ui, new_ui) -> bool:
        """
        Set the name of the Pwnagotchi

        :return: If the command was sent successfully
        """
        if not _ui_diff(current_ui=current_ui, new_ui=new_ui, key='name'):
            return True

        logging.info(f"[PwnZero] ui differs, setting name")
        name = new_ui.get('name').replace(">", "")
        return self._send_string(FlipperCommand.UI_NAME.value, name)

    def set_channel(self, current_ui, new_ui) -> bool:
        """
        Set the channel of the Pwnagotchi

        :return: If the command was sent successfully
        """

        if not _ui_diff(current_ui=current_ui, new_ui=new_ui, key='channel'):
            return True
        logging.info(f"[PwnZero] ui differs, setting channel")
        channel = new_ui.get('channel')
        return self._send_string(FlipperCommand.UI_CHANNEL.value, channel)

    def set_aps(self, current_ui, new_ui) -> bool:
        """
        Set the APs of the Pwnagotchi

        :return: If the command was sent successfully
        """

        if not _ui_diff(current_ui=current_ui, new_ui=new_ui, key='aps'):
            return True
        logging.info(f"[PwnZero] ui differs, setting aps")
        #TODO fix ap handling
        aps = new_ui.get('aps')
        # logging.info(f"[PwnZero] pwnzero aps = {aps}")
        #:param: apsCurrent: Number of APS this session
        #:param: apsTotal: Number of APS in unit lifetime

        # return self._send_string(FlipperCommand.UI_APS.value, "{} ({})".format(apsCurrent, apsTotal))

        return True

    def set_uptime(self, current_ui, new_ui) -> bool:
        """
        Sets the uptime ui element of the Pwnagotchi on the flipper ui

        :return: If the command was sent successfully
        """

        if not _ui_diff(current_ui=current_ui, new_ui=new_ui, key='uptime'):
            return True

        logging.info(f"[PwnZero] ui differs, setting uptime")
        uptime = new_ui.get('uptime')
        uptimeSplit = uptime.split(':')

        hh = int(uptimeSplit[0])
        mm = int(uptimeSplit[1])
        ss = int(uptimeSplit[2])

        # Make sure all values are less than 100 and greater than 0
        if not (0 <= hh < 100 and 0 <= mm < 100 and 0 <= ss < 100):
            return False

        # A stands for adjusted
        hhA = str(hh).zfill(2)
        mmA = str(mm).zfill(2)
        ssA = str(ss).zfill(2)

        return self._send_string(FlipperCommand.UI_UPTIME.value, "{}:{}:{}".format(hhA, mmA, ssA))

    def set_friend(self, current_ui, new_ui) -> bool:
        """
        Friend is currently not supported

        :return: False
        """
        # if not _ui_diff(current_ui=current_ui, new_ui=new_ui, key='friend'):
        #     return True
        # logging.info(f"[PwnZero] ui differs, setting friend")
        return True

    def set_mode(self, current_ui, new_ui) -> bool:
        """
        Set the mode ui element of the Pwnagotchi on the flipper ui

        :return: If the command was sent successfully
        """

        if not _ui_diff(current_ui=current_ui, new_ui=new_ui, key='mode'):
            return True

        logging.info(f"[PwnZero] ui differs, setting mode")
        mode = None
        if new_ui.get('mode') == 'AI':
            mode = PwnMode.AI
        elif new_ui.get('mode') == 'MANU':
            mode = PwnMode.MANU
        elif new_ui.get('mode') == 'AUTO':
            mode = PwnMode.AUTO

        return self._send_bytes(FlipperCommand.UI_MODE.value, [mode.value])

    def set_handshakes(self, current_ui, new_ui) -> bool:
        """
        Set the number of handshakes ui element of the Pwnagotchi on the flipper ui

        :param: handshakesCurrent: Number of handshakes this session
        :param: handshakesTotal: Number of handshakes in the lifetime of unit
        :return: If the command was sent successfully
        """

        if not _ui_diff(current_ui=current_ui, new_ui=new_ui, key='shakes'):
            return True

        logging.info(f"[PwnZero] ui differs, setting handshakes")
        #TODO fix handshakes
        handshakes = new_ui.get('shakes')
        # logging.info(f"[PwnZero] handshakes = {handshakes}")

        # shakesCurr = handshakes.split(' ')[0]
        # shakesTotal = handshakes.split(' ')[1].replace(')', '').replace('(', '')

        # return self._send_string(FlipperCommand.UI_HANDSHAKES.value, "{} ({})".format(handshakesCurrent, handshakesTotal))

        return True

    def set_status(self, current_ui, new_ui) -> bool:
        """
        Sets the displayed status ui element of the Pwnagotchi on the flipper ui

        :param: status: Status to set
        :return: If the command was sent successfully
        """
        if not _ui_diff(current_ui=current_ui, new_ui=new_ui, key='status'):
            return True
        logging.info(f"[PwnZero] ui differs, setting status")
        status = new_ui.get('status')
        logging.info(f"[PwnZero] status: {status}")
        #TODO reformat to fix flipper screen size restrictions first?
        return self._send_string(FlipperCommand.UI_STATUS.value, status)


class PwnZero(plugins.Plugin):
    __author__ = "github.com/Matt-London, eva@evaemmerich.com"
    __version__ = "2.0.0"
    __license__ = "MIT"
    __description__ = "Plugin to the integrate Pwnagotchi into the Flipper Zero"


    def __init__(self):
        self._flipper = Flipper()
        # when running is True, we should continue looking for a flipper to chat with
        # when false, we should shut down
        self.running = False
        self.connected = False
        # when receiving bytes, if error_count reaches
        # max_error, we leave the connected loop and
        # return to trying to synack
        self.error_count = 0
        self.max_error = 5

        self.synack_sleep = 5


        self.new_ui = None
        self.current_ui = None


    def on_loaded(self):
        logging.info(f"[PwnZero] plugin loaded")
        self._flipper.open_serial()

        self.running = True

        logging.info(f"[PwnZero] starting syn loop")
        while self.running:
            # TODO backoff how aggressively we syn over time
            time.sleep(self.synack_sleep)
            # synack
            try:
                self._flipper.send_syn()
            except Exception as e:
                # we don't care much about exceptions when initialy trying to establish communication
                logging.info(f"[PwnZero] error while attemption to synack: {type(e).__name__}:{e.args}")
            else:
                logging.info(f"[PwnZero] synack complete, flipper connected!")
                self.connected = True
                self.error_count = 0
                self.current_ui = None
                while self.connected:
                    # main communication loop

                    # send ui updates
                    self._flipper.update_ui(self.current_ui, self.new_ui)
                    self.current_ui = self.new_ui

                    # receive commands from flipper
                    try:
                        msg = self._flipper.receive_bytes()
                    except PwnZeroSerialException as e:
                        logging.info(f"[PwnZero] receive exception in main loop: {type(e).__name__}:{e.args}")
                        self.error_count += 1
                        if self.error_count == self.max_error:
                            logging.info(f"[PwnZero] max_error count {self.max_error} reached, disconnecting")
                            self.connected = False
                    except ReceivedNone:
                        # nothing to do
                        pass
                    else:
                        # we have some command to handle!
                        logging.info(f"[PwnZero] received flipper message: {msg}")

                        if msg[0] == PwnCommand.SYN.value:
                            self._flipper.send_ack()
                        elif msg[0] == PwnCommand.UI_REFRESH.value:
                            self.current_ui = None
                            self._flipper.send_ack()
                        else:
                            logging.info(f"[PwnZero] received flipper message, but not able to handle command.: {msg}")
                            self._flipper.send_nak()

    def on_unload(self):
        self.connected = False
        self.running = False

        self._flipper.close_serial()

    def on_ui_setup(self, ui):
        pass

    def on_ui_update(self, ui):
        logging.debug("[PwnZero] on_ui_update")
        self.new_ui = ui

    def on_rebooting(self):
        pass

    def _receive_command(self):
        pass

    def cmd_reboot(self):
        # mode = 'MANU' if agent.mode == 'manual' else 'AUTO'
        # pwnagotchi.reboot(mode=mode)
        pass