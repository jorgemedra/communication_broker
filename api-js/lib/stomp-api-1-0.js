/*
http://localhost:7771/
http://localhost:8161/admin/index.jsp

https://stomp.github.io/
*/

//var CFG_STOMP_SECRET_KEY = "THIS IS MY SECRET KEY TO ACCESS";
var CFG_STOMP_SECRET_KEY = "my_app_secret_key";

class stomp_commands
{
    static CMD_CONNECT = "CONNECT";
    static CMD_CONNECTED = "CONNECTED";
    static CMD_DISCONNECT = "DISCONNECT";
    static CMD_RECEIPT = "RECEIPT";
    static CMD_SEND = "SEND";
    static CMD_MESSAGE = "MESSAGE";
    static CMD_SUBSCRIBE = "SUBSCRIBE";
    static CMD_UNSUBSCRIBE = "UNSUBSCRIBE";
    static CMD_BEGIN = "BEGIN";
    static CMD_COMMIT = "COMMIT";
    static CMD_ABORT = "ABORT";
    static CMD_ACK = "ACK";
    static CMD_NACK = "NACK";
    static CMD_ERROR = "ERROR";
}//class stomp_commands

class stomp_heders
{
    static HDR_ACCEP_VERSION = "accept-version";
    static HDR_ERROR_DESC ="error-desc";
    static HDR_LOGIN = "login";
    static HDR_DEST_TYPE = "destination-type";
    static HDR_DEST = "destination";
    static HDR_ORIGIN ="origin";
    static HDR_PASSCODE ="passcode";
    static HDR_SECRET_KEY = "secret-key";
    static HDR_SESSION_ID = "session-id";
    static HDR_PROF_CODE = "profile-code";
    static HDR_PROF_DESC = "profile-desc";
    static HDR_CNX_ID = "connection-id";

    static HDR_TRANSACTION = "transaction";
    static HDR_RECEIPT_ID = "receipt-id";

    static HDR_CONT_LENGHT = "content-length";
    static HDR_CONT_TYPE = "content-type";  
}//class stomp_heders

class STOMPMessage
{
    constructor()
    {
        this.Command = "";
        //this.Headers = [];
        this.Headers = [];
        this.Body = null;
    }


}//class STOMPMessage


class STOMPClient {

    constructor(idClient, wsurl){
        self = this;
        this.LF = '\x0A';
        this.NULL = '\x00';

        this.cur_session = "";
        this.cur_destination = "";
        this.id_client = 0;
        this.m_id = idClient;
        this.m_tid = "[" + idClient + "] ";
        this.m_debug = false;
        this.url = wsurl;

        this._usr = "";
        this._pwd = "";

        this.LG_INFO = 0;
        this.LG_WARN = 1;
        this.LG_ERROR = 2;
        this.LG_DEBUG = 3;

        this.ws = null;

        this.fb_onOpen = null;
        this.fb_onClosed = null;
        this.fb_onError = null;
        this.fb_onMessage = null;

    };

    NextId()
    {
        if(this.id_client == 1000)
            this.id_client = 0;
        this.id_client++;
        return this.id_client;
    }

    linkEvents(fb_onOpen, fb_onClosed, fb_onError, fb_onMessage)
    {
        this.fb_onOpen = fb_onOpen;
        this.fb_onClosed = fb_onClosed;
        this.fb_onError = fb_onError;
        this.fb_onMessage = fb_onMessage;
    }

    /* Log Events */
    writeLog(message, level)
    {
        if(level == this.LG_DEBUG && this.m_debug == false)
        {
            return;
        }

        var ld = new Date();
        var l_time = ld.getHours() + ":" + ld.getMinutes() + ":" + ld.getSeconds() + "." + ld.getMilliseconds() + "\t";

        var l_msg = l_time + this.m_tid + message;
        
        if(level == this.LG_INFO)
        {
            console.info(l_msg);
        }
        else if(level == this.LG_WARN)
        {
            console.warn(l_msg);
        }
        else if(level == this.LG_ERROR)
        {
            console.error(l_msg);
        }
        else if(level == this.LG_DEBUG)
        {
            console.debug(l_msg);
        }
    }

    w_info(message)
    {   
        this.writeLog(message, this.LG_INFO);
    }

    w_warn(message)
    {   
        this.writeLog(message, this.LG_WARN);
    }

    w_error(message)
    {   
        this.writeLog(message, this.LG_ERROR);
    }

    w_debug(message)
    {   
        this.writeLog(message, this.LG_DEBUG);
    }

    activeDebug()
    {
        this.m_debug = true;
    }

    deactiveDebug()
    {
        this.m_debug = false;
    }

    /************************************************************
    Events 
    *************************************************************/
    onOpen()
    {
        if(self.m_debug == true)
        {
            self.w_debug("WS OPEN.");  
        }
        self._connectToServer();        
    }

    onClosed(event)
    {
        if(self.m_debug == true)
        {
            self.w_debug("WS CLOSED CODE: " + event.code);
        }
        var reason = "";
        if (event.code == 1000)
            reason = "Normal closure, meaning that the purpose for which the connection was established has been fulfilled.";
        else if(event.code == 1001)
            reason = "An endpoint is \"going away\", such as a server going down or a browser having navigated away from a page.";
        else if(event.code == 1002)
            reason = "An endpoint is terminating the connection due to a protocol error";
        else if(event.code == 1003)
            reason = "An endpoint is terminating the connection because it has received a type of data it cannot accept (e.g., an endpoint that understands only text data MAY send this if it receives a binary message).";
        else if(event.code == 1004)
            reason = "Reserved. The specific meaning might be defined in the future.";
        else if(event.code == 1005)
            reason = "No status code was actually present.";
        else if(event.code == 1006)
           reason = "The connection was closed abnormally, e.g., without sending or receiving a Close control frame";
        else if(event.code == 1007)
            reason = "An endpoint is terminating the connection because it has received data within a message that was not consistent with the type of the message (e.g., non-UTF-8 [https://www.rfc-editor.org/rfc/rfc3629] data within a text message).";
        else if(event.code == 1008)
            reason = "An endpoint is terminating the connection because it has received a message that \"violates its policy\". This reason is given either if there is no other sutible reason, or if there is a need to hide specific details about the policy.";
        else if(event.code == 1009)
           reason = "An endpoint is terminating the connection because it has received a message that is too big for it to process.";
        else if(event.code == 1010) // Note that this status code is not used by the server, because it can fail the WebSocket handshake instead.
            reason = "An endpoint (client) is terminating the connection because it has expected the server to negotiate one or more extension, but the server didn't return them in the response message of the WebSocket handshake. <br /> Specifically, the extensions that are needed are: " + event.reason;
        else if(event.code == 1011)
            reason = "A server is terminating the connection because it encountered an unexpected condition that prevented it from fulfilling the request.";
        else if(event.code == 1015)
            reason = "The connection was closed due to a failure to perform a TLS handshake (e.g., the server certificate can't be verified).";
        else
            reason = "Unknown reason";

        self.fb_onClosed(event.code, reason);
    }

    onError(e)
    {
        if(self.m_debug == true)
        {
            self.w_debug('WebSocket Error');
        }
        self.fb_onError(e);
    }

    //TODO: Change the message that is delivered.
    onMessage(e)
    {
        if(self.m_debug == true)
        {
            self.w_debug(">>onMessage:\n[------------------------------------------------------------\n"  
                    + e.data + 
                    "\n------------------------------------------------------------]");
        }

        var msg = self._parseMessage(e.data);

        if(msg.Command == "CONNECTED")
        {
            self.cur_session = msg.Headers[stomp_heders.HDR_SESSION_ID];
            self.cur_destination = msg.Headers[stomp_heders.HDR_DEST];
            self.fb_onOpen(this.cur_session);
        }
        else if(msg.Command == "MESSAGE")
        {
            self.fb_onMessage(msg.Headers, msg.Body);
        }
        
    }

    /***********/


    connectToServer(usr, pwd)
    {
        
        self.w_debug("Connection to " + this.url);
        if(this.ws != null)
        {
            this.w_warn("This class is already connected.");
            return;
        }
        
        this.ws = new WebSocket(this.url, ['stomp']);
        
        this._usr = usr;
        this._pwd = pwd;

        this.ws.onopen = this.onOpen;
        this.ws.onclose = this.onClosed;
        this.ws.onmessage = this.onMessage;
        this.ws.onerror = this.onError;
    }

    _sendraw(data)
    {
        this.w_debug("<<Sending:\n------------------------------\n"  
                    + data + 
                    "\n------------------------------");
        this.ws.send(data);
    }
    
/**************************************** 
 * STOMP 
 ****************************************/

    buildSTOMPMessage(command, headers, body)
    {
        var data = command + this.LF;
        
        for(var i = 0; i< headers.length; i++)
            data = data + headers[i] + this.LF;

        data = data + this.LF + body + this.NULL;        
        //data = data + this.NULL;
        return data;
    }

    _readline(data, from_pos)
    {
        var posline = data.indexOf("\n", from_pos);
        if(posline >= 0)
        {
            var line = data.substring(from_pos, posline);
            return [line, posline+1];
        }
        return ["" , data.length];
    }

    _parseMessage(data)
    {
        var msg = new STOMPMessage();
        var pos = 0;
        var result = this._readline(data, pos);
        
        msg.Command = result[0];
        pos = result[1];

        while(pos < data.length)
        {
            result = this._readline(data, pos);
            var line= result[0];
            pos = result[1];

            if (line != "")
            {
                var idx = line.indexOf(':');
                var K = line.substring(0,idx);
                var V = line.substring(idx + 1);
                msg.Headers[K.trim()] = V.trim();
            }
            else{
                //pos = pos + 1; // read the Data
                if(pos < data.length)
                {
                    msg.Body = data.substring(pos);
                    msg.Body = msg.Body.substring(0, msg.Body.length-1);                    
                }
                break;
            }
        }

        return msg;
    }

    _connectToServer()
    {
        const headers = [
            "accept-version  :  1.2",
            //"heart-beat: 5000,5000",
            "login:  " + this._usr,
            "passcode  :" + this._pwd,
            "secret-key : " + CFG_STOMP_SECRET_KEY
        ];

        var data = this.buildSTOMPMessage("CONNECT", headers, "");
        this._sendraw(data);
    }

    diconnectToServer()
    {
        const headers = [
            stomp_heders.HDR_RECEIPT_ID + ":" + this.NextId()
        ];

        var data = this.buildSTOMPMessage(stomp_commands.CMD_DISCONNECT, headers, "");
        this._sendraw(data);
        //this.ws.close();
        this.ws = null;
    }
    
    //suscribe(id, destiny)
    suscribe(destiny)
    {
        const headers = [
            stomp_heders.HDR_TRANSACTION + ":" + this.NextId(),
            stomp_heders.HDR_SESSION_ID + ":" + this.cur_session,
            stomp_heders.HDR_DEST_TYPE + ":1",
            stomp_heders.HDR_DEST + ":" + destiny
        ];

        var data = this.buildSTOMPMessage(stomp_commands.CMD_SUBSCRIBE, headers, "");
        this._sendraw(data);
    }

    //unsuscribe(id)
    unsuscribe()
    {
        const headers = [
            stomp_heders.HDR_TRANSACTION + ":" + this.NextId(),
            stomp_heders.HDR_SESSION_ID + ":" + this.cur_session
        ];

        var data = this.buildSTOMPMessage(stomp_commands.CMD_UNSUBSCRIBE, headers, "");
        this._sendraw(data);
    }

    sendText(destiny, body)
    {
        const headers = [
            stomp_heders.HDR_TRANSACTION + ":" + this.NextId(),
            stomp_heders.HDR_SESSION_ID + ":" + this.cur_session,
            stomp_heders.HDR_DEST + ":" + destiny,
            stomp_heders.HDR_CONT_TYPE + ": text/plain",
            stomp_heders.HDR_CONT_LENGHT + ": " + body.length
        ];

        var data = this.buildSTOMPMessage(stomp_commands.CMD_SEND, headers, body);
        this._sendraw(data);
    }
}// class STOMPClient
