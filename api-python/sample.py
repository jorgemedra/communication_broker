from concurrent.futures import thread
import sys
import threading
from com_broker import broker_client 

__keepRunning__ = True

#1. Creating
__app_key__ = "my_app_secret_key"
__agent_prof_key__ = "my_agent_secret_key"
__sagent_prof_key__ = "my_super_agent_secret_key"
__service_prof_key__ = "my_service_secret_key"
__admin_prof_key__ = "my_admin_secret_key"
__cert_path__ = "cert/server.crt"


client = broker_client("Client-1", __app_key__)

def connect(login_id, profile, use_ssl=False):
    if client.is_connected():
        print("The client is already connected.")
        return
    
    if use_ssl:
        client.set_ssl_options(cert_path=__cert_path__)
    
    #Connecting and Login
    client.link_call_back_functions(on_connected=ev_connected, 
                                    on_disconnected=ev_diconnected,
                                    on_error=ev_error,
                                    on_message=ev_message,
                                    on_ack=ev_ack)
    keyprof = ""
    if profile == "agn":
        keyprof = __agent_prof_key__
    elif profile == "sagn":
        keyprof = __sagent_prof_key__
    elif profile == "srv":
        keyprof = __service_prof_key__
    elif profile == "adm":
        keyprof = __admin_prof_key__
    else:
        keyprof = __agent_prof_key__
    
    client.connect(host="localhost", port=6662, server_hostname="MacBook-Pro-de-Jorge.local",
                    login_id=login_id, secret_key=keyprof)
    

def disconnect():
    if client.is_connected():
        client.disconnect()
    else:
        print("The client is not connected.")

def send(destination, message):
    if not client.is_connected():
        print("The client is not connected.")
        return
    
    print("Sending to[{0}] the message: [{1}]".format(destination, message))
    client.send_to(destination, message)

def subscribe(destination, use_regex):
    if not client.is_connected():
        print("The client is not connected.")
        return

    print("Subscribing to [{0}], Type of Regex:[{1}].".format(destination, use_regex))
    client.subscribe(destination, use_regex)

def unsubscribe():
    if not client.is_connected():
        print("The client is not connected.")
        return

    print("UnSubscribing.")
    client.unsubscribe()

'''
************************************************
Events
    --- BEGIN ---
************************************************
'''

def ev_connected(session_id, loginid, destination_id):
    print('''
[SAMPLE] ON CONNECTED EVENT:
    Session ID:[{0}]
    Loginid:[{1}]
    Destination:[{2}]
'''.format(session_id, loginid,  destination_id))


def ev_diconnected(session_id):
    print('''
[SAMPLE] ON DISCONNECTED EVENT:
    Session ID:[{0}]
'''.format(session_id))
    sys.exit(0)


def ev_diconnect():
    print("Cliente Disconnected")


def ev_error():
    print("Cliente Error")
    

def ev_message(origin, destination, content_type, payload):
    print('''
[SAMPLE]  >>>>>>> On Message Event - RX MESSAGE (BEGIN):
FROM:[{0}]
TO: [{1}]
Content-Type:[{2}]
PAYLOAD
<------------->
{3}
<------------->
<<<<<<< RX MESSAGE (END)
'''.format(origin,destination,content_type, payload))

def ev_ack(transaction_id, error_desc):
    print('''
[SAMPLE]  On ACK Event :
    TRANS ID:[{0}]
    ERROR DESC:[{1}]
'''.format(transaction_id, error_desc))
    


'''
************************************************
Events
    --- END ---
************************************************
'''
  
def help():
    promt = '''
------------------------------
Commands:
    quit = end program
    cnx <login id> <profile: agn|sagn|srv|adm>= connect with an specific logind ID and profile
    sub <destination> <regeb =[1|0]> = Subcribe to destination
    unsub = Subcribe to destination
    send <destination> <message> = send a message to an specific destination
'''
    print(promt)


if __name__ == "__main__":
    
    help()
    while __keepRunning__:
        command = input("$ ")
        commands = command.split()
        
        if command == '' or command == "help":
            help()
        elif __keepRunning__ and command == "quit":
            disconnect()
            __keepRunning__ = False
        elif __keepRunning__ and commands[0] == "cnx":
            connect(commands[1], commands[2], True)
        elif __keepRunning__ and commands[0] == "send":
            if len(commands) >= 3:
                msg = ""
                bIni = True
                for data in commands[2:]:
                    if bIni:
                        bIni = False
                    else:
                        msg += " "
                    msg += data
                send(commands[1],  msg)
            else:
                help()
        elif __keepRunning__ and commands[0] == "sub":
            if len(commands) >= 3:
                use_regex = True if command[3] == "1" else False
                subscribe(commands[1], use_regex)
            else:
                help()
        elif __keepRunning__ and commands[0] == "unsub":
            unsubscribe()
        else:
            help()
        
        
