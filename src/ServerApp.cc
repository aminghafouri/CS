/*
 * P2PApp.cc
 *
 *  Created on: Sep 22, 2012
 *      Author: Aniruddha Gokhale
 *      Class:  CS381
 *      Institution: Vanderbilt University
 */

#include "ClientServerAppMsg_m.h"    // generated header from the message file
#include "ServerApp.h"         // our header

#include "IPvXAddressResolver.h"

Define_Module(ServerApp);


ServerApp::ServerApp (void)
{
    // nothing
}

ServerApp::~ServerApp()
{
    // nothing
}



// the initialize method
void ServerApp::initialize (int stage)
{
    std::cout << "Server started initialization" << endl;
    cSimpleModule::initialize (stage);
    if (stage != 3)
        return;

    // obtain the values of parameters
    this->localAddress_ = this->par ("localAddress").stringValue ();
    this->localPort_ = this->par ("localPort");
    //this->connectPort_ = this->par ("connectPort");
    // here is a way to read a parameter array (see user manual section 4.5.4)
    //const char *str = this->par ("connectAddresses").stringValue ();
    //this->connectAddresses_ = cStringTokenizer (str).asVector();

    string dataTransferMode = this->par ("dataTransferMode").stringValue ();

    // create a new socket for the listening role
    this->socket_ = new TCPSocket ();

    // decide the data transfer mode
    if (!dataTransferMode.compare ("bytecount"))
        this->socket_->setDataTransferMode (TCP_TRANSFER_BYTECOUNT);
    else if (!dataTransferMode.compare ("object"))
        this->socket_->setDataTransferMode (TCP_TRANSFER_OBJECT);
    else if (!dataTransferMode.compare ("bytestream"))
        this->socket_->setDataTransferMode (TCP_TRANSFER_BYTESTREAM);
    else { // error
        EV << "=== Bad data transfer mode value. Assuming object" << endl;
        this->socket_->setDataTransferMode (TCP_TRANSFER_OBJECT);
    }

    // In the server role, we bind ourselves to the well-defined port and IP address on which
    // we listen for incoming connections
    this->socket_->bind (this->localAddress_.length () ?
            IPvXAddressResolver ().resolve (this->localAddress_.c_str ()) : IPvXAddress (),
            this->localPort_);

    // register ourselves as the callback object
    bool *passive = new bool (true);
    this->socket_->setCallbackObject (this, passive);  // send the flag

    // do not forget to set the outgoing gate
    this->socket_->setOutputGate (gate ("tcpOut"));

    // now listen for incoming connections.  This version is the forking version where
    // upon every new incoming connection, a new socket is created.
    this->socket_->listen ();

    // debugging
    //EV << "+++ Peer: " << this->localAddress_ << " created a listening socket with "
    //   << "connection ID = " << this->socket_->getConnectionId () << " +++" << endl;

    // now save this socket in our map
    this->socketMap_.addSocket (this->socket_);


    setStatusString ("waiting");
    std::cout << "Server finished initialization" << endl;
}

/** the all serving handle message method */
void ServerApp::handleMessage (cMessage *msg)
{
    std::cout << "Server recieved msg" << endl;
    EV << "=== Peer: " << this->localAddress_ << " received handleMessage message" << endl;

    // check if this was a self generated message, such as a timeout to wake us up to
    // connect to a peer

        // let the socket class process the message and make a call back on the
        // appropriate method. But note that we need to determine which socket must
        // handle this message: it could be connection establishment in our passive role
        // or it could be ack to our active conn establishment or it could be a data packet
        TCPSocket *socket = this->socketMap_.findSocketFor(msg);
        if (!socket) {
            // we are going to be here if we are a passive listener of incoming connection
            // at which point a connection will be established. But there will not be a
            // socket created yet for the data transfer. We create such a socket.

            // make sure first that we are dealing with a TCP command
            TCPCommand *cmd = dynamic_cast<TCPCommand *>(msg->getControlInfo());
            if (!cmd) {
                throw cRuntimeError("ServerApp::handleMessage: no TCPCommand control info in message (not from TCP?)");
            } else {
                EV << "=== Peer: " << this->localAddress_ << " **No socket yet ** ===" << endl;

                int connId = cmd->getConnId();
                // debugging
                EV << "+++ Peer: " << this->localAddress_ << " creating a new socket with "
                   << "connection ID = " << connId << " ===" << endl;

                // notice that we must use the other constructor of TCPSocket so that it
                // will use the underlying connID that was created after an incoming
                // connection establishment message
                TCPSocket *new_socket = new TCPSocket (msg);

                // register ourselves as the callback object
                bool *passive = new bool (true);
                new_socket->setCallbackObject (this, passive);

                // do not forget to set the outgoing gate
                new_socket->setOutputGate (gate ("tcpOut"));

                // another thing I learned the hard way is that we must set up the data trasnfer
                // mode for this new socket
                new_socket->setDataTransferMode (this->socket_->getDataTransferMode ());

                // now save this socket in our map
                this->socketMap_.addSocket (new_socket);

                // process the message
                std::cout << "Server processing msg (new)" << endl;
                new_socket->processMessage (msg);
            }
        } else {
            // let that socket process the message
            std::cout << "Server processing msg" << endl;
            socket->processMessage (msg);

        }

}

/** this method is provided to clean things up when the simulation stops */
void ServerApp::finish ()
{
    std::cout << "Server finished" << endl;
    EV << "=== finish called" << endl;

    // cleanup all the sockets
    this->socketMap_.deleteSockets ();

    std::string modulePath = getFullPath();
}


/** handle the timeout method */
void ServerApp::handleTimer (cMessage *msg)
{
    std::cout << "Server handling timer" << endl;
    EV << "=== Peer: " << this->localAddress_ << " received handleTimer message. "
       << "Make a connection to " << this->numPeers_ << " peers === " << endl;

    // cleanup the incoming message
    delete msg;

    // make connections to all our peers
    for (int i = 0; i < this->numPeers_; ++i) {
        // for each peer we issue a connect
        this->connect (i);
    }
}



/*************************************************/
/** implement all the callback interface methods */
/*************************************************/

void ServerApp::socketEstablished (int connID, void *role)
{
    std::cout << "Socket established" << endl;
    EV << "=== Peer: " << this->localAddress_
       << " received socketEstablished message on connID " << connID << " ===" << endl;

    setStatusString("ConnectionEstablished");

    // now that the connection is established, if we are the active entity that opens a
    // connection, then we initiate the next data transfer with the peer
    //bool *passive = static_cast<bool *> (role);
    //////////// amin: CHANGED
    bool *passive = static_cast<bool *> (role);
        if (*passive) {
            EV << "=== We are in passive role and hence just wait for an incoming req ===" << endl;
        } else {
            EV << "=== We are in active role and hence initiate a req ===" << endl;

            // send requests to the peer to whom we just got connected
            this->sendRequest (connID, this->localAddress_.c_str (), "dummy.txt");
        }

}

/** handle incoming data. Could be a request or response */
void ServerApp::socketDataArrived(int connID, void *, cPacket *msg, bool)
{
    std::cout << "Server socked data arrived" << endl;
    EV << "=== Peer: " << this->localAddress_
       << " received socketDataArrived message. ===" << endl;

    // incoming request may be of different types
    CS_Packet *packet = dynamic_cast<CS_Packet *> (msg);
    if (!packet) {
        return;
    }


    switch ((CS_MSG_TYPE)packet->getType ()) {
    case CS_REQUEST:
    {
        CS_Req *req = dynamic_cast<CS_Req *> (msg);
        if (!req) {
            EV << "Arriving packet is not of type P2P_Req" << endl;
        } else {
            setStatusString ("Request");
            //EV << "Arriving packet: Requestor ID = " << req->getId ()
            //   << ", Requested filename = " << req->getFile ()  << endl;

            // now send a response

            this->sendResponse (connID, this->localAddress_.c_str (), 256);
        }
    }
    break;
    default:
        EV << ">>> unknown incoming request type <<< " << endl;
        break;
    }
    // cleanup
    delete msg;
}

void ServerApp::socketPeerClosed (int connID, void *)
{
    std::cout << "Server socket peer closed" << endl;
    EV << "=== Peer: " << this->localAddress_ << " received socketPeerClosed message" << endl;
    EV << "peer closed for connID = " << connID << endl;
    ////////////////////////// remove the socket from socketmap
}

void ServerApp::socketClosed (int, void *)
{
    std::cout << "Server socket closed" << endl;
    EV << "=== Peer: " << this->localAddress_ << " received socketClosed message" << endl;
    // *redefine* to start another session etc.
    EV << "connection closed\n";
    setStatusString("closed");
}

void ServerApp::socketFailure (int, void *, int code)
{
    std::cout << "Server socket failure" << endl;
    EV << "=== Peer: " << this->localAddress_ << " received socketFailure message" << endl;
    // subclasses may override this function, and add code try to reconnect after a delay.
    EV << "connection broken\n";
    setStatusString("broken");

}

/**********************************************************************/
/**           helper methods                                          */
/**********************************************************************/

// connect to peer i
void ServerApp::connect (int i)
{
    std::cout << "Server connect" << endl;
    EV << "=== Peer: " << this->localAddress_ << " received connect message" << endl;
    EV << "issuing connect command\n";
    setStatusString ("connecting");

    // we allocate a socket to be used for actively connecting to the peer and
    // transferring data over it.
    TCPSocket *new_socket = new TCPSocket ();

    // don't forget to set the output gate for this socket. I learned it the
    // hard way :-(
    new_socket->setOutputGate (gate ("tcpOut"));

    // another thing I learned the hard way is that we must set up the data transfer
    // mode for this new socket
    new_socket->setDataTransferMode (this->socket_->getDataTransferMode ());

    // issue a connect request
    new_socket->connect (IPvXAddressResolver().resolve (this->connectAddresses_[i].c_str ()),
                         this->connectPort_);

    // do not forget to set ourselves as the callback on this new socket
    bool *passive = new bool (false);
    new_socket->setCallbackObject (this, passive);

    // debugging
    EV << "+++ Peer: " << this->localAddress_ << " created a new socket with "
       << "connection ID = " << new_socket->getConnectionId () << " ===" << endl;

    // save this socket in our outgoing connection map
    this->socketMap_.addSocket (new_socket);

    // also save the number of times we have made the request thus far for this
    // active socket. Remember this is a map that maps a socket* to num requests made.

}

// close the peer side
void ServerApp::close()
{
    std::cout << "Server close" << endl;
    EV << "=== Peer: " << this->localAddress_ << " received close () message" << endl;
    EV << "issuing CLOSE command\n";

    setStatusString("closing");

    this->socket_->close ();
}

void ServerApp::sendRequest (int connId, const char *id, const char *fname)
{

}

// send a response
void ServerApp::sendResponse (int connId, const char *id, unsigned long size)
{
    std::cout << "Server sending response" << endl;
    EV << "=== Peer: " << this->localAddress_ << " sendResponse. "
       << "Sending ID: " << id << ", size: " << size << " ===" << endl;

    // this is a hack because the TCPSocketMap does not allow us to search based on
    // connection ID. So we have to take a circuitous route to get to the socket
    cMessage *temp_msg = new cMessage ("temp");
    TCPCommand *temp_cmd = new TCPCommand ();
    temp_cmd->setConnId (connId);
    temp_msg->setControlInfo (temp_cmd);

    TCPSocket *socket = this->socketMap_.findSocketFor (temp_msg);
    if (!socket) {
        EV << ">>> Cannot find socket to send request <<< " << endl;
    } else {
        CS_Resp *resp = new CS_Resp ();
        resp->setType ((int)CS_RESPONSE);
        resp->setId (id);
        resp->setDataArraySize(size);
        // need to set the byte length else nothing gets sent as I found the hard way
        resp->setByteLength (1024);  // I think we can set any length we want :-)

        socket->send (resp);
    }

    // cleanup
    delete temp_msg;
}


void ServerApp::setStatusString(const char *s)
{
    if (ev.isGUI ()) {
        getDisplayString ().setTagArg ("t", 0, s);
        bubble (s);
    }
}

