
var amq = null;

// Secret App Key
var __app_key__ = "my_app_secret_key"

// Secret Profile Keys
var __agent_prof_key__ = "my_agent_secret_key"
var __sagent_prof_key__ = "my_super_agent_secret_key"
var __service_prof_key__ = "my_service_secret_key"
var __admin_prof_key__ = "my_admin_secret_key"

function iniForm()
{   
    $("#btnConnect").show();
    $("#btnClose").hide();
    $("#div-logged").hide();


    $("#btnSuscribe").hide();
    $("#btnUnSuscribe").hide();
    $("#chkTopic").prop("checked", false);

    
}

/**********************************************************
* Controls
**********************************************************/

function OpenConnection()
{
    WriteLog("Opening Connection.");

    var url = $("#txtURL").val();
    var usr = $("#txtUser").val();
    var prof = $("#opt_prof:checked").val();
    var pwd = "";
    
    if(prof == "agn")
        pwd = __agent_prof_key__;
    else if(prof == "sagn")
        pwd = __sagent_prof_key__;
    else if(prof == "srv")
        pwd = __service_prof_key__;
    else if(prof == "adm")
        pwd = __admin_prof_key__;


    $("#btnConnect").hide();
    
    amq =new COM_BROKER_Client("CLT-1", url);
    amq.activeDebug();
    amq.linkEvents(OnConnected, onClosed, onError, onMessage, onACK);
    amq.connect(usr, pwd);
}

function CloseConnection()
{
    WriteLog("Closing Connection.");
    amq.diconnectToServer();
}

function CloseConnection()
{
    amq.diconnectToServer();
}

function OnType()
{
    var chk = $("#chkTopic").prop("checked");
    if(chk)
    {
        $("#lbType").text("Topic");
    }
    else
    {
        $("#lbType").text("Queue");
    }
}

function Suscribe()
{
    WriteLog("Suscribing.");
    var temp = $("#chkRegex").prop("checked");
    var use_regextype = temp ? "1" : "0";
    var dest = $("#txtDestSuscribe").val();
    amq.subscribe(dest, use_regextype);
}

function Send()
{
    WriteLog("Sending.");
    var dest = $("#txtDestination").val();
    var msg = $("#txtTxMessage").val();
    var content_type = "text/plain";

    amq.send(dest, msg, content_type);
}

function Clear()
{
    $("#txtRxMessage").val("");
}

function ClearLogs()
{
    $("#txtLogs").val("Logs...");
}

function WriteLog(logmessage)
{
    var log = $("#txtLogs").val();
    $("#txtLogs").val(log + "\n" + logmessage + "\n");
}

function Unsuscribe()
{
    WriteLog("Unscribing.");
    amq.unsubscribe();
}

/**********************************************************
* EVENTS
**********************************************************/
function OnConnected(session_id, destination_id, profile, cnx_id)
{
    WriteLog("Connection opened.");

    $("#btnConnect").hide();
    $("#btnClose").show();

    $("#div-logged").show();

    $("#session_id").text(session_id);
    $("#destination_id").text(destination_id);
    $("#profile").text(profile);
    $("#cnx_id").text(cnx_id);

    $("#btnSuscribe").show();
    $("#btnUnSuscribe").show();

    $("#txtDestSuscribe").val(destination_id);
}

function onClosed(session_id, code, reason)
{
    WriteLog("Connection closed with id [" + session_id + "] by Code(" + code + "): " + reason);

    $("#btnConnect").show();
    $("#btnClose").hide();
    $("#div-logged").hide();

    $("#destination_id").text("");

    $("#btnSuscribe").hide();
    $("#btnUnSuscribe").hide();
    amq = null;
}

function onError(error)
{
    WriteLog("ON ERROR: " + error);
}

function onMessage(origin, destination, content_type, payload)
{   
    hdrs_info = "On Message:\n";
    hdrs_info += "\tOrigin:" + origin + "\n" ;
    hdrs_info += "\tDestination:" + destination + "\n" ;
    hdrs_info += "\tContent Type:" + content_type + "\n" ;

    WriteLog("On Message: HEADERS: " + hdrs_info);

    var info = $("#txtRxMessage").val(); 
    $("#txtRxMessage").val(info + "--------BEGIN MESSAGE--------\n" + payload + "\n--------END MESSAGE--------\n");
}

function onACK(data)
{
    WriteLog("ON ACK: " + data);
}