
#import asyncio
#from asyncore import write
#from curses.ascii import NUL
#import asyncio
#from select import select
#from warnings import catch_warnings
from pprint import pprint
import socket
import ssl
from telnetlib import TLS
import threading
from turtle import Turtle

from matplotlib.pyplot import disconnect

class stomp_protocol:
    CMD_CONNECT = "CONNECT"
    CMD_CONNECTED = "CONNECTED"
    CMD_DISCONNECT = "DISCONNECT"
    CMD_RECEIPT = "RECEIPT"
    CMD_SEND = "SEND"
    CMD_MESSAGE = "MESSAGE"
    CMD_SUBSCRIBE = "SUBSCRIBE"
    CMD_UNSUBSCRIBE = "UNSUBSCRIBE"
    CMD_BEGIN = "BEGIN"
    CMD_COMMIT = "COMMIT"
    CMD_ABORT = "ABORT"
    CMD_ACK = "ACK"
    CMD_NACK = "NACK"
    CMD_ERROR = "ERROR"

    HDR_ACCEP_VERSION = "accept-version"
    HDR_ERROR_DESC = "error-desc"
    HDR_LOGIN = "login"
    HDR_DEST_TYPE = "destination-type"
    HDR_DEST = "destination"
    HDR_ORIGIN = "origin"
    HDR_PASSCODE = "passcode"
    HDR_SECRET_KEY = "secret-key"
    HDR_SESSION_ID = "session-id"
    HDR_PROF_CODE = "profile-code"
    HDR_PROF_DESC = "profile-desc"
    HDR_CNX_ID = "connection-id"
    HDR_TRANSACTION = "transaction"
    HDR_RECEIPT_ID = "receipt-id"
    HDR_CONT_LENGHT = "content-length"
    HDR_CONT_TYPE = "content-type"
    
class stomp_builder:
    
    #_CH_END = 0x00
    _CH_END = '\0'
    _CH_NL = '\n'
    
    def __init__(self):
        pass
    
    def build_message(self, command, headers, payload):
        msg = "{0}{1}".format(command, self._CH_NL)
        
        for hdr in headers:
            msg += "{0}:{1}{2}".format(hdr, headers[hdr], self._CH_NL)
        
        #msg += "{0}{1}{2}".format(self._CH_NL, payload, self._CH_END)
        msg += "{0}{1}".format(self._CH_NL, payload)
        return msg
    
    def get_line(self, data, pos):

        if pos >= len(data):
            return [-1,""]

        npos = data.find('\n', pos)
        ret_data = ""
        
        if npos > 0:
            re_data = data[pos : npos]
            
        return [npos+1, re_data]
    
    def parse_header(self, line, pos):
        '''
        [valid=True|False, key, value header]
        '''
        index = line.find(':')
        if index < 0:
            return[False,"",""]
        
        key = line[:index]
        value = line[index + 1:]
        
        return [True, key.strip(), value.strip()]
        
    def parse_message(self, data):
        
        msg = stomp_message()

        if len(data) == 0:
            return msg
        
        pos = 0
        [npos, line]= self.get_line(data, pos)
        pos = npos
        
        if npos < 0:
            return msg
        else:
            msg.command = line
        
        
        [npos, line] = self.get_line(data, pos)
        pos = npos
        
        while line != "" and npos > 0:
            [valid, key, value] = self.parse_header(line, pos)
            if not valid:
                return stomp_message()
            msg.headers[key] = value
            [npos, line] = self.get_line(data, pos)
            pos = npos
            
        msg.payload = data[pos: len(data)-1]
        
        return msg
    
class stomp_message:
    def __init__(self):
        self.command = ""
        self.headers = {}
        self.payload = ""
    
class broker_client:
    
    def __init__(self, id, app_key):
        self._VERSION_ = "Communication Broker API V1.0"
        self._RECEPIT_DSCNX_ = "DISCONNECTED"
        self.id = id
        self.session_id = id
        self.app_key = app_key
        self.login_id = ""
        self.destination_id = ""
        self.secret_key = ""
        self.is_ssl = False
        self.cer_file = ""
        self.host = "localhost"
        self.port = 0
        self.connected = False
        self.logged = False
        self.buffer_tx = "";
        self.buffer_rx = "";
        self.trans_id = 0
        
        self.sck = None
        self.thr_rx = None
        #self.reader = None
        #self.writer = None
        self.on_connected = None
        self.on_disconnect = None
        self.on_error = None
        self.on_message = None
        self.on_ack = None
    
    def version(self):
        return self._VERSION_
    
    def _debug(self, msg):
        msgd = "[{0}]\t{1}".format(self.id,msg)
        print(msgd)
        
    def link_call_back_functions(self, on_connected, on_disconnected, on_error, on_message, on_ack):
        self.on_connected = on_connected
        self.on_disconnected = on_disconnected
        self.on_error = on_error
        self.on_message = on_message
        self.on_ack = on_ack
    
    def set_app_key(self, app_key):
        self.app_key = app_key
        
    def set_ssl_options(self, cert_path):
        self.is_ssl = True
        self.cer_file = cert_path
    
    def handle_error(self):
        print("handle_error")
    
    def _trans_id(self):
        self.trans_id += 1
        if self.trans_id > 10000:
            self.trans_id = 0
            
        return self.trans_id
    
    def is_connected(self):
        return self.connected
    
    def connect(self, server_hostname, host, port, login_id, secret_key):
        if self.connected == True:
            return

        #self.loop = asyncio.get_running_loop()
        self.host = host
        self.port = int(port)
        self.login_id = login_id
        self.secret_key = secret_key
        
        self._debug("Connection to {0}:{1}".format(self.host, self.port))
        if self.is_ssl:
            # self.context = context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
            # self.context.load_verify_locations(self.cer_file)
            
            sck = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
            # self.sck = ssl.wrap_socket(sck,
            #                            ca_certs= self.cer_file,
            #                            ssl_version=ssl.PROTOCOL_TLSv1_2,
            #                            do_handshake_on_connect=True,
            #                            server_side=False,
            #                            cert_reqs= ssl.CERT_REQUIRED)
            
            self.context = ssl.SSLContext(ssl.PROTOCOL_TLSv1_2)
            self.sck = self.context.wrap_socket(sck, do_handshake_on_connect=True, server_side=False)
            
            # self.sck = self.context.wrap_socket(sck, server_hostname=server_hostname)
            
            
        else:
            self.sck = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            
            
        self.sck.connect((self.host, self.port))
        self.connected = True
        
        if self.is_ssl:
            print("Socket Version: {0}".format(self.sck.version()))            
            print("Cypher: {0}".format(self.sck.cipher()))
            cert = self.sck.getpeercert()
            pprint(cert)
        
        self.thr_rx = threading.Thread(target=self.wait_for_data)
        self.thr_rx.start()
        self._login()
                    
    def _login(self):
        print("_login")
        headers = {
            stomp_protocol.HDR_ACCEP_VERSION : "1.2",
            stomp_protocol.HDR_LOGIN: self.login_id,
            stomp_protocol.HDR_PASSCODE: self.secret_key,
            stomp_protocol.HDR_SECRET_KEY: self.app_key
        }

        builder = stomp_builder()
        self.buffer_tx = builder.build_message(command=stomp_protocol.CMD_CONNECT, headers=headers, payload="")
        self._write_()
         
    def disconnect(self):
        if self.connected == False:
            return
        
        headers = {
            stomp_protocol.HDR_RECEIPT_ID: self._RECEPIT_DSCNX_
        }

        builder = stomp_builder()
        self.buffer_tx = builder.build_message(
            command=stomp_protocol.CMD_DISCONNECT, headers=headers, payload="")
        self._write_()
    
    def subscribe(self, destination, use_regex):
        #print("Send To")
        dest_type = 1 if use_regex else 0
         
        headers = {
            stomp_protocol.HDR_TRANSACTION: self._trans_id(),
            stomp_protocol.HDR_SESSION_ID: self.session_id,
            stomp_protocol.HDR_ORIGIN: self.login_id,
            stomp_protocol.HDR_DEST_TYPE: dest_type,
            stomp_protocol.HDR_DEST: destination
        }

        builder = stomp_builder()
        self.buffer_tx = builder.build_message(
            command=stomp_protocol.CMD_SUBSCRIBE, headers=headers, payload="")
        self._write_()

    def unsubscribe(self):
        headers = {
            stomp_protocol.HDR_TRANSACTION: self._trans_id(),
            stomp_protocol.HDR_SESSION_ID: self.session_id
        }

        builder = stomp_builder()
        self.buffer_tx = builder.build_message(
            command=stomp_protocol.CMD_UNSUBSCRIBE, headers=headers, payload="")
        self._write_()
    
    def send(self, destination, message, content_type="text/plain"):
        #print("Send To")
        headers = {
            stomp_protocol.HDR_TRANSACTION :self._trans_id(),
            stomp_protocol.HDR_SESSION_ID: self.session_id,
            stomp_protocol.HDR_ORIGIN: self.login_id,
            stomp_protocol.HDR_DEST: destination,
            stomp_protocol.HDR_CONT_TYPE: content_type,
            stomp_protocol.HDR_CONT_LENGHT: len(message)
        }

        builder = stomp_builder()
        self.buffer_tx = builder.build_message(command=stomp_protocol.CMD_SEND, headers=headers, payload=message)
        
        #print("SENDING[\n{0}\n]".format(self.buffer_tx))
        self._write_()
    
    def _write_(self):
        buffer_size = len(self.buffer_tx)
        # band to set the Little is firt on output stream.
        buffer = False.to_bytes(length=1, byteorder="little", signed=False)
        buffer += buffer_size.to_bytes(length=4, byteorder="little", signed=False)
        buffer += str.encode(self.buffer_tx)
        self.sck.send(buffer)
    
    def _read_(self, amount):
        try:
            if self.connected:
                buffer = self.sck.recv(amount)
                if len(buffer) == 0:
                    buffer = None
                    self._end_connection_()
                return buffer
        except (OSError) as err:
            print("Reading exception: {0}".format(err))
            
        return None
    
    def wait_for_data(self):
        while self.connected:
            #print("wait for data...")
            # try:
            data = self._read_(1)
            if data == None:
                break
            bInt = bool.from_bytes(bytes=data, byteorder="little", signed=False)
            
            data = self._read_(4)
            if data == None:
                break
            size = int.from_bytes(bytes=data, byteorder="little", signed=False)

            data = self._read_(size)
            if data == None:
                break
            msg = str(data[0:size], encoding="utf-8")
            #print( "handle_read: DATA:\n****************\n{0}\n****************".format(msg))
            self._process_command(msg)
    
    def _end_connection_(self):
        self.connected = False
        self.logged = False
        session_id = self.session_id
        self.id = ""
        self.login_id = ""
        self.destination_id = ""

        self.sck.close()
        self.on_disconnected(session_id)
    
    def get_msg_info(self, msg: stomp_message):
        info = "[{0}]".format(msg.command)
        for key in msg.headers.keys():
            info += "\n\t[{0}]:[{1}]".format(key,msg.headers[key])        
        info += '''
PAYLOAD:
******* BEGIN *******
{0}
******* END *******
'''.format(msg.payload)
        return info
    
    def _process_command(self, data):
        
        parser = stomp_builder()        
        msg = parser.parse_message(data)
        
        if msg.command == "":
            self._debug("Invalid Message.\n[{0}]".format(data))
            return

        #self._debug("Message Info:\n{0}".format(self.get_msg_info(msg)))
        if msg.command == stomp_protocol.CMD_CONNECTED:
            self._process_connected(msg)
        elif msg.command == stomp_protocol.CMD_RECEIPT:
            self._process_receipt(msg)
        elif msg.command == stomp_protocol.CMD_MESSAGE:
            self._process_message(msg)
        elif msg.command == stomp_protocol.CMD_ACK:
            self._process_ack(msg)
        else:
            print('''
Uknown Message from Comm Server
{0}
'''.format(self.get_msg_info(msg)))
            
    def _process_connected(self, msg:stomp_message):
        self.logged = True
        self.session_id = msg.headers[stomp_protocol.HDR_SESSION_ID]
        self.login_id = msg.headers[stomp_protocol.HDR_LOGIN]
        profile = msg.headers[stomp_protocol.HDR_PROF_DESC]
        cnx_id = msg.headers[stomp_protocol.HDR_CNX_ID]
        self.destination_id = msg.headers[stomp_protocol.HDR_DEST]
        
        self.on_connected(self.session_id, self.destination_id, profile, cnx_id)
    
    def _process_receipt(self, msg: stomp_message):
        
        receipt_id = msg.headers[stomp_protocol.HDR_RECEIPT_ID]
        
        if receipt_id == self._RECEPIT_DSCNX_:            
            self._debug("The connection has been disconnected.")
            self._end_connection_()
        else:
            self._debug("Receipt ID [{0}] has been processed.".format(receipt_id))
    
    def _process_message(self, msg: stomp_message):
        origin = msg.headers[stomp_protocol.HDR_ORIGIN]
        destination = msg.headers[stomp_protocol.HDR_DEST]
        content_type = msg.headers[stomp_protocol.HDR_CONT_TYPE]
        self.on_message(origin, destination, content_type, msg.payload)
            
    def _process_ack(self, msg: stomp_message):
        trans_id = msg.headers[stomp_protocol.HDR_TRANSACTION]
        error_desc = msg.headers[stomp_protocol.HDR_ERROR_DESC]
        self.on_ack(trans_id, error_desc)


