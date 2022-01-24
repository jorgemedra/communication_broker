
var amq = null;

function iniForm()
{   
    $("#btnConnect").show();
    $("#btnClose").hide();

    $("#btnSuscribe").hide();
    $("#btnUnSuscribe").hide();

    $("#chkTopic").prop("checked", false);
}

/**********************************************************
* Controls
**********************************************************/

function OpenConnection()
{
    console.log("Opening Connection.");
    var url = $("#txtURL").val();
    var usr = $("#txtUser").val();
    var pwd = $("#txtPassword").val();
    
    $("#btnConnect").hide();
    
    amq =new STOMPClient("STOMP CLT-1", url);
    amq.activeDebug();
    amq.linkEvents(OnConnected, onClosed, onError, onMessage);
    amq.connectToServer(usr, pwd);
}

function CloseConnection()
{   console.log("Closing Connection.");
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
    console.log("Suscribing.");
    //$("#txtID").val(amq.NextId());
    // var isTopic = $("#chkTopic").prop("checked");
    //var id = amq.NextId(); //$("#txtID").val();
    // var susId = $("#txtDestSuscribe").val();

    //var type = isTopic ? "/topic/" : "/queue/";
    //var dest = type + susId;
    // var dest = susId;
    var dest = $("#txtDestSuscribe").val();
    amq.suscribe(dest);

}

function Send()
{
    console.log("Sending.");
    //$("#txtID").val(amq.NextId());
    //var isTopic = $("#chkTopic").prop("checked");
    // var susId = $("#txtDestiny").val();

    //var type = isTopic ? "/topic/" : "/queue/";
    //var dest = type + susId;
    // var dest = susId;

    var dest = $("#txtDestiny").val();
    var msg = $("#txtTxMessage").val();

    amq.sendText(dest, msg);
}

function Unsuscribe()
{
    console.log("Unscribing.");    
    // var id = $("#txtID").val();
    amq.unsuscribe();
}

/**********************************************************
* EVENTS
**********************************************************/
function OnConnected(destination_id)
{
    console.log("Connection opened.");

    $("#btnConnect").hide();
    $("#btnClose").show();

    $("#btnSuscribe").show();
    $("#btnUnSuscribe").show();

    $("#txtDestSuscribe").val(destination_id);
}


function onClosed(code, reason)
{
    console.log("Connection closed by Code(" + code + "): " + reason);
    $("#btnConnect").show();
    $("#btnClose").hide();

    $("#btnSuscribe").hide();
    $("#btnUnSuscribe").hide();
    amq = null;
}

function onError(e)
{
    console.error("!!!!" + e);
}

function onMessage(headers, data)
{   
    console.log("OnMessage: HEADERS: " + headers);
    //console.log("OnMessage: DATA: " + data);

    var info = $("#txtRxMessage").val(); 

    $("#txtRxMessage").val(info + "--------BEGIN MESSAGE--------\n" + data + "\n--------END MESSAGE--------\n");

}