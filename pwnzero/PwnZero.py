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
    The values are the bytes that are being sent to the Flipper Zero
    """
    # Control Values
    SYN   = 0x16
    ACK   = 0x06

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
    These commands can be sent from the Flipper to the pwnagotchi
    """
    # Control Values
    SYN   = 0x16
    ACK   = 0x06

    # pwnagotchi commands
    REBOOT         = 0x04
    SHUTDOWN       = 0x05
    MODE           = 0x07
    UI_REFRESH     = 0x08 # request a ui refresh from the pwnagotchi

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

class Flipper():

    def __init__(self, port: str = "/dev/serial0", baud: int = 115200):
        """
        Construct a Flipper object, this will create the connection to the flipper

        :param: port: Port on which the UART of the Flipper is connected to
        :param: baud: Baudrate for communication to the Flipper (default 115200)
        """

        # rather than have to keep a list of all of our ui setters, generate one
        self._ui_setters = [method for method in dir(self) if callable(getattr(self, method)) if method.startswith('set_')]

        self._port = port
        self._baud = baud

        self._serial_conn = None

    def _str_to_bytes(self, s: str):
        """
        Converts a string into a list of bytes

        :param: s: String to convert
        :return: List of bytes
        """
        retVal = []
        for c in s:
            retVal.append(ord(c))

        return retVal

    def _send_string(self, cmd: int, msg_string: str) -> bool:
        byte_encoded_string = self._str_to_bytes(msg_string)
        return self._send_bytes(cmd, byte_encoded_string)

    def _send_bytes(self, cmd: int, body: [int]) -> bool:
        """
        Sends a packet using protocol v3 over the serial port to the Flipper Zero

        :param: cmd: Parameter that is being changed
        :param: body: Arguments to pass to the flipper. Must be a list of bytes
        :return: If transmission was successful
        """
        # Build the packet to send
        packet = [Packet.START, cmd] + body + [Packet.END]

        # Send data to flipper
        return self._serial_conn.write(body) == len(packet)

    def connect(self):
        try:
            self._serial_conn = serial.Serial(self._port, self._baud)
        except:
            raise "Cannot bind to port ({}) with baud ({})".format(self._port, self._baud)

    def disconnect(self):
        # clean up the serial connection
        self._serial_conn.close()


    def update_ui(self, ui) -> bool:
        """
        Set the ui elements of the Pwnagotchi
        Calls all Flipper methods that start with "set_"

        :return: If any ui setter fails, log the failure and return False
        """
        all_ret = True
        for method in self._ui_setters:
            ret = getattr(self, method)(ui)  # call
            if not ret:
                logging.error(f"{method} failed")
                all_ret = False

        return all_ret

    def set_face(self, ui) -> bool:
        """
        Set the face of the Pwnagotchi

        :return: If the command was sent successfully
        """

        face = ui.get('face')

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

        return self._send_bytes(FlipperCommand.UI_FACE.value, [face.value])

    def set_name(self, ui) -> bool:
        """
        Set the name of the Pwnagotchi

        :return: If the command was sent successfully
        """
        name = ui.get('name').replace(">", "")
        return self._send_string(FlipperCommand.UI_NAME.value, name)

    def set_channel(self, ui) -> bool:
        """
        Set the channel of the Pwnagotchi

        :return: If the command was sent successfully
        """

        channel = ui.get('channel')
        return self._send_string(FlipperCommand.UI_CHANNEL.value, channel)

    def set_aps(self, ui) -> bool:
        """
        Set the APs of the Pwnagotchi

        :return: If the command was sent successfully
        """

        #TODO fix ap handling
        aps = ui.get('aps')
        logging.info(f"pwnzero aps = {aps}")
        #:param: apsCurrent: Number of APS this session
        #:param: apsTotal: Number of APS in unit lifetime

        # return self._send_string(FlipperCommand.UI_APS.value, "{} ({})".format(apsCurrent, apsTotal))

        return True

    def set_uptime(self, ui) -> bool:
        """
        Sets the uptime ui element of the Pwnagotchi on the flipper ui

        :return: If the command was sent successfully
        """

        uptime = ui.get('uptime')
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

    def set_friend(self) -> bool:
        """
        Friend is currently not supported

        :return: False
        """
        return True

    def set_mode(self, ui) -> bool:
        """
        Set the mode ui element of the Pwnagotchi on the flipper ui

        :return: If the command was sent successfully
        """
        mode = None
        if ui.get('mode') == 'AI':
            mode = PwnMode.AI
        elif ui.get('mode') == 'MANU':
            mode = PwnMode.MANU
        elif ui.get('mode') == 'AUTO':
            mode = PwnMode.AUTO

        return self._send_bytes(FlipperCommand.UI_MODE.value, [mode.value])

    def set_handshakes(self, ui) -> bool:
        """
        Set the number of handshakes ui element of the Pwnagotchi on the flipper ui

        :param: handshakesCurrent: Number of handshakes this session
        :param: handshakesTotal: Number of handshakes in the lifetime of unit
        :return: If the command was sent successfully
        """

        #TODO fix handshakes
        handshakes = ui.get('shakes')
        logging.info(f"pwnzero handshakes = {handshakes}")

        # shakesCurr = handshakes.split(' ')[0]
        # shakesTotal = handshakes.split(' ')[1].replace(')', '').replace('(', '')

        # return self._send_string(FlipperCommand.UI_HANDSHAKES.value, "{} ({})".format(handshakesCurrent, handshakesTotal))

        return True

    def set_status(self, ui) -> bool:
        """
        Sets the displayed status ui element of the Pwnagotchi on the flipper ui

        :param: status: Status to set
        :return: If the command was sent successfully
        """
        status = ui.get('status')
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


    def on_loaded(self):
        self._flipper.connect()

        #TODO tell the flipper we are ready, ensure it responds properly
        # try to complete a connection
        # while self.running:
        #     time.sleep(1)

            # TODO

            # send hello

            # wait for reply
            # if no reply, wait and send hello again

            # if reply, set connected=True and move to waiting for message loop

    def on_unload(self):
        self.connected = False
        self.running = False

        self._flipper.disconnect()


    def on_ui_setup(self, ui):
        pass

    def on_ui_update(self, ui):
        self._flipper.update_ui(ui)

    def _receive_command(self):
        pass

    def cmd_reboot(self):
        # mode = 'MANU' if agent.mode == 'manual' else 'AUTO'
        # pwnagotchi.reboot(mode=mode)
        pass