/*
 * ClientApp.cc
 *
 *  Created on: Sep 22, 2012
 *      Author: Aniruddha Gokhale
 *      Class:  CS381
 *      Institution: Vanderbilt University
 */

#include "ClientServerAppMsg_m.h"     // generated header from the message file
#include "ClientApp.h"         // our header

#include "IPvXAddressResolver.h"

Define_Module(ClientApp);


ClientApp::ClientApp (void)
: fileSize_ (),
  socket_ (NULL),
  id_()
{

}

ClientApp::~ClientApp()
{
    // nothing
}



// the initialize method
void ClientApp::initialize (int stage)
{
    std::cout << id_ << " initialization started" <<  endl;
    cSimpleModule::initialize (stage);
    if (stage != 3)
        return;

    // obtain the values of parameters
    //this->localAddress_ = this->par ("localAddress").stringValue ();
    //this->localPort_ = this->par ("localPort");
    //this->numPeers_ = this->par ("numPeers");
    this->connectPort_ = this->par ("connectPort");
    // here is a way to read a parameter array (see user manual section 4.5.4)
    this->connectAddress_ = this->par ("connectAddress").stringValue ();
    this->fileSize_ = this->par ("fileSize");
    this->id_ = this->par ("id").stringValue ();
    //this->numRequests_ = this->par ("numRequests");
    //this->reqSize_ = this->par ("reqSize");
    //this->respSize_ = this->par ("respSize");

    // indicate what type of data transfer is going to be supported by our socket
    // there are three choices supported in INET. For now we choose OBJECT, i.e.,
    // the underlying system will send the fields and we indicate some size. If we had
    // chosen BYTECOUNT, then the system emulates sending of that many bytes. We
    // are not concerned with the actual content. But if we really wanted to do it
    // that way, then we will do BYTESTREAM
    string dataTransferMode = this->par ("dataTransferMode").stringValue ();

    this->dataTransferMode_= TCP_TRANSFER_OBJECT;

    // now we start a timer so that when it kicks in, we make a connection to another peer
    cMessage *timer_msg = new cMessage ("timer");
    this->scheduleAt (simTime () + exponential (0.001), timer_msg);

    setStatusString ("waiting");
    //EV <<"Datatransfer Mode of Client:" <<dataTransferMode <<endl;
    std::cout << id_ << " initialization done" << endl;
}

/** the all serving handle message method */
void ClientApp::handleMessage (cMessage *msg)
{
    std::cout << "Client started handling message" << endl;
    EV << "=== Peer: " << this->localAddress_ << " received handleMessage message" << endl;

    // check if this was a self generated message, such as a timeout to wake us up to
    // connect to a peer
    if (msg->isSelfMessage ())
        this->handleTimer (msg);
    else {
            // let that socket process the message
            socket_->processMessage (msg);
        }
    std::cout << "Client done handling message" << endl;
}

/** this method is provided to clean things up when the simulation stops */
void ClientApp::finish ()
{
    std::cout << "Client finished" << endl;
    std::cout << "=== finish called" << endl;

    // cleanup all the sockets
    //this->socketMap_.deleteSockets ();

    std::string modulePath = getFullPath();

    delete this->socket_;
}


/** handle the timeout method */
void ClientApp::handleTimer (cMessage *msg)
{
    std::cout << "Client handling timer" << endl;
    EV << "=== Peer: " << this->localAddress_ << " received handleTimer message. "
       << "Make a connection to " << " peers === " << endl;

    // cleanup the incoming message
    delete msg;

    this->connect ();


}

/*************************************************/
/** implement all the callback interface methods */
/*************************************************/

void ClientApp::socketEstablished (int connID, void *role)
{
    std::cout << "Client socket established" << endl;
    EV << "=== Peer: " << this->localAddress_
       << " received socketEstablished message on connID " << connID << " ===" << endl;

    setStatusString("ConnectionEstablished");

    // now that the connection is established, if we are the active entity that opens a
    // connection, then we initiate the next data transfer with the peer
    //bool *passive = static_cast<bool *> (false);
    if (role != NULL) {
        EV << "=== We are in passive role and hence just wait for an incoming req ===" << endl;
    } else {
        EV << "=== We are in active role and hence initiate a req ===" << endl;

        // send requests to the peer to whom we just got connected
        this->sendRequest (connID);
    }
}

/** handle incoming data. Could be a request or response */
void ClientApp::socketDataArrived(int connID, void *, cPacket *msg, bool)
{
    std::cout << "Client socket arrived" << endl;
    //EV << "=== Peer: " << this->localAddress_
    //   << " received socketDataArrived message. ===" << endl;

    // incoming request may be of different types
    CS_Resp *response = dynamic_cast<CS_Resp *> (msg);
    if (!response) {
        return;
    }
    else {
        std::cout << "Client handling response" << endl;
        this->handleResponse (response);
    }
        /*

    switch ((P2P_MSG_TYPE)packet->getType ()) {
    case P2P_REQUEST:
    {
        P2P_Req *req = dynamic_cast<P2P_Req *> (msg);
        if (!req) {
            EV << "Arriving packet is not of type Client_Req" << endl;
        } else {
            setStatusString ("Request");
            EV << "Arriving packet: Requestor ID = " << req->getId ()
               << ", Requested filename = " << req->getFile ()  << endl;

            // now send a response
            this->sendResponse (connID, this->localAddress_.c_str (), 128);
        }
    }
    break;
    case P2P_RESPONSE:
    {
        P2P_Resp *resp = dynamic_cast<P2P_Resp *> (msg);
        if (!resp) {
            EV << "Arriving packet is not of type Client_Resp" << endl;
        } else {
            setStatusString ("Response");
            EV << "Arriving packet: Responder ID = " << resp->getId ()
               << ", size = " << resp->getSize () << endl;

            // send the next request to the peer from whom we got a response
            this->sendRequest (connID);
        }
    }
    break;
    default:
        EV << ">>> unknown incoming request type <<< " << endl;
        break;
    }
    */
    // cleanup
    delete response;
    this->close();
}

void ClientApp::socketPeerClosed (int connID, void *)
{
    std::cout << "Client peer closed" << endl;
    //EV << "=== Peer: " << this->localAddress_ << " received socketPeerClosed message" << endl;
    EV << "peer closed for connID = " << connID << endl;

}

void ClientApp::socketClosed (int, void *)
{
    std::cout << "Client socket closed" << endl;
    //EV << "=== Peer: " << this->localAddress_ << " received socketClosed message" << endl;
    // *redefine* to start another session etc.
    delete this->socket_;
    EV << "connection closed\n";
    setStatusString("closed");
}

void ClientApp::socketFailure (int, void *, int code)
{
    std::cout << "Client socket failure" << endl;
    //EV << "=== Peer: " << this->localAddress_ << " received socketFailure message" << endl;
    // subclasses may override this function, and add code try to reconnect after a delay.
    EV << "connection broken\n";
    setStatusString("broken");
    //endSimulation();

}

/**********************************************************************/
/**           helper methods                                          */
/**********************************************************************/

// connect to peer i
void ClientApp::connect (void)
{
    std::cout << "Client connect" << endl;
    //EV << "=== Peer: " << this->localAddress_ << " received connect message" << endl;
    EV << "issuing connect command\n";
    setStatusString ("connecting");

    // we allocate a socket to be used for actively connecting to the peer and
    // transferring data over it.
    this->socket_ = new TCPSocket ();

    // don't forget to set the output gate for this socket. I learned it the
    // hard way :-(
    this->socket_->setOutputGate (gate ("tcpOut"));

    // another thing I learned the hard way is that we must set up the data transfer
    // mode for this new socket //
    this->socket_->setDataTransferMode (this->dataTransferMode_);

    // issue a connect request
    this->socket_->connect(IPvXAddressResolver().resolve (this->connectAddress_.c_str ()),
            this->connectPort_);

    // do not forget to set ourselves as the callback on this new socket
    this->socket_->setCallbackObject (this, NULL);

    // debugging
    //EV << "+++ Peer: " << this->localAddress_ << " created a new socket with "
    //   << "connection ID = " << socket_->getConnectionId () << " ===" << endl;

    // save this socket in our outgoing connection map
    //this->socketMap_.addSocket (new_socket);

    // also save the number of times we have made the request thus far for this
    // active socket. Remember this is a map that maps a socket* to num requests made.
    //this->reqPerSocketMap_[new_socket] = this->numRequests_;
}

// close the peer side
void ClientApp::close()
{
    std::cout << "Client close" << endl;
    //EV << "=== Peer: " << this->localAddress_ << " received close () message" << endl;
    EV << "issuing CLOSE command\n";

    setStatusString("closing");

    this->socket_->close ();
}

// send a request to the other side
void ClientApp::sendRequest (int connId)
{
    std::cout << "Client send request" << endl;
    //EV << "=== Peer: " << this->localAddress_ << " sendRequest. " << " ===" << endl;

    // this is a hack because the TCPSocketMap does not allow us to search based on
    // connection ID. So we have to take a circuitous route to get to the socket
        // check if we have not completed the number of requests sent
    // amin amin amin
            //EV << "&&&& Peer #" << this->localAddress_ << endl;
            CS_Req *req = new CS_Req ();
            req->setType (int(CS_REQUEST));
            //req->setId (id);
            req->setFilesize (this->fileSize_);
            // need to set the byte length else nothing gets sent as I found the hard way
            req->setByteLength (sizeof (CS_MSG_TYPE) + sizeof (this->fileSize_));  // I think we can set any length we want :-)

            this->socket_->send (req);

}

void ClientApp::handleResponse (CS_Resp *response)
{

    std::cout << "message received from server" << std::endl;
    for (unsigned int i = 0; i < response->getDataArraySize (); ++i) {
            EV << response->getData (i);
        }

    //

}
/*
// send a response
void ClientApp::sendResponse (int connId, const char *id, unsigned long size)
{
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
        P2P_Resp *resp = new P2P_Resp ();
        resp->setType ((int)P2P_RESPONSE);
        resp->setId (id);
        resp->setSize (size);
        // need to set the byte length else nothing gets sent as I found the hard way
        resp->setByteLength (this->respSize_);  // I think we can set any length we want :-)

        socket->send (resp);
    }

    // cleanup
    delete temp_msg;
}

*/
void ClientApp::setStatusString(const char *s)
{
    if (ev.isGUI ()) {
        getDisplayString ().setTagArg ("t", 0, s);
        bubble (s);
    }
}

