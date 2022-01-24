# PREVIEW

The *STOMP Server* provides a broker message service for several clients who needs an easy way to send information without concern about the basic handle sessions and map. Its design is based on Message Queue, but not being as one. Its features are:

1. Provide a WebSocket library to be used on web clients, which is implemented on JS.
2. The basic protocol is STOMP, adapted to its process.
3. Work with profiles and secret keys, instead of user login control. It means that you can sign any user on the service as long as the private keys are available on the Third-Party application.
4. Its design is focused on providing service, the user validation is delegated to the Third-Party application.
5. The destination id, the id wich is used to receive messages from another client, is create and register at the moment of sign.

# Profiles

There are four profiles:

All the profiles have the rights to send message to another destination, the main difference is the rights to subscribe to a foreigner destination.

1. **Agent**. The sinlgest profile. It doesn't have the rights to suscribe to another destination. This profile has the same prefix for destination than an Super-Agent Profile.
2. **Super Agent**. It has the rigst to suscribe to Agent Destination but not others. This profile has the same prefix for destination than an Agent Profile.
3. **Service**. It has the abilitie to suscribe to any othre profile of Agent or Super Agent, to receive their message. Also, it´s possible to receive events Login and Logof of of each Agent/SuperAgent.
4. **Administrator**. This profile only allows connections which are dedicated for Monitoring and Controling the StopServer.

# Rules for loggin

## Agent/Super Agent

1. On a valid connection with a valid *Anget-Id* and *Profile*, and If there is a previous session logged with the same *Agent-Id* and profile, the new connection will disconect the previous connection allowing the new connection takinf the place of previous connection.
2. On a loggin try, if the Application-Secret-Key or Profile-Secret-Key is wrong, the connection will be disconnected.

## Service/Admin

1. If there is a previous session with the same *Agent-Id* to the new connection, the new connection will be droped and keep the previous connection.  

# Subscriptions

As *STOMP Server* is designed to have the option of receiving the message of other destinations. the *STOMP Server* allows to connections wich have a profile of *Super Agent* and *Service* to susribe to some foreigner destination of Agent or Service, according to the rules of profiles.

# Event Listening

Only the services that have a *Service Profile* can susbcribe to events of *Agents* or *Super Agents*, these events are:

1. **Logged**. A new session with a new *Agent-Id* has been logged.
2. **Disconnected**. A session has been logged.
3. **Logout**. All the sessions of an *Agent-Id* has been removed, so the agent is no longer signed.

To be suscribed to these events, the client, of *Service Profile*, must send an **SUBSCRIBE** message to the destination **STOMP-SERVER**, and destination-id with a *regex* expression to filtrate the source of event, according to its destination, e.g:

1. Any *Agent+ with a *login-id* numeric: `^\/agent\/\d+$`
2. Any *Super Agent* with an *login-id* alpha numeric: `^\/agent\/\w+$`
3. Any *Super Agent* with an specific *login-id*: `/agent\/MyID`.
4. Any *Agent* with a *login-id* that starts with weather `o` or `m`: `^/agent/[om]\w*$`.
5. Any *Agent* with a *destination-id* that starts with */cobranza/* and has only numbers: `^\/agent\/cobranza/\d+$` 

> **Note: To avoid hit the performance of the stomp server, it should avoid complex regex patterns to select the destination.**

To unsubscribe, send and **UNSUBSCRIBE** Message with the same destination header and value.


## Regular Expression

```cmd
. Any single character (a "wildcard") 
\ Next character has a special meaning
+ One or more
∗ Zero or more
[ Begin character class
] End character class
{ Begin count
} End count
? Optional (zero or one)
| Alternative (or)
( Begin grouping
) End grouping 
ˆ Start of line; negation
$ End of line
```