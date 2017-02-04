/*
 * P2PApp.h
 *
 *  Created on: Sep 22, 2012
 *      Author: Aniruddha Gokhale
 *      Class:  CS381
 *      Institution: Vanderbilt University
 */

#ifndef SERVERAPP_H_
#define SERVERAPP_H_

#include <string>
#include <vector>
using namespace std;

#include "INETDefs.h"       // this contains imp definitions from the INET
#include "TCPSocket.h"      // this is needed for sockets
#include "TCPSocketMap.h"   // this is needed to maintain multiple connected
                            // sockets from other peers


/**
 * This is our application class that demonstrates a P2P capability.
 *
 * Each peer must have both the client and server capabilities.  Since we may be
 * connected to a number of peers while many peers may connect to us, we are going
 * to maintain a socket map for outgoing and incoming connections
 */
class ServerApp : public cSimpleModule, public TCPSocket::CallbackInterface
{

  public:
    /**
     *  constructor
     */
    ServerApp (void);

    /**
     * destructor
     */
    virtual ~ServerApp (void);

  private:

    TCPSocket    *socket_;   // our main listening socket
    TCPSocketMap socketMap_; // maps of sockets we maintain

    string localAddress_;    // our local address
    int localPort_;          // our local port we will listen on

    int numPeers_;           // indicates how many peers we are to connect to
    vector<string> connectAddresses_;  // address of our peers
    int connectPort_;        // ports of the peer we connect to

    /*
    int numRequests_;        // number of requests to send to each peer
    int reqSize_;            // size of each request message
    int respSize_;           // size of each response message

    typedef map<TCPSocket *, int> ReqPerSocketMap;
    ReqPerSocketMap reqPerSocketMap_;   // maintains a map of num of requests sent per socket
     */
  protected:
    /**
     * Initialization. Should be redefined to perform or schedule a connect().
     */
    virtual void initialize (int stage);

    /**
     * define how many initialization stages are we going to need.
     */
    virtual int numInitStages (void) const { return 4; }

    /**
     * For self-messages it invokes handleTimer(); messages arriving from TCP
     * will get dispatched to the socketXXX() functions.
     */
    virtual void handleMessage (cMessage *msg);

    /**
     * Records basic statistics: numSessions, packetsSent, packetsRcvd,
     * bytesSent, bytesRcvd. Redefine to record different or more statistics
     * at the end of the simulation.
     */
    virtual void finish();

    /** Invoked from handleMessage(). Should be defined to handle self-messages. */
    virtual void handleTimer (cMessage *msg);

    /** @name Utility functions */

    //@{
    /** Issues an active OPEN to the address/port given as module parameters */
    virtual void connect (int i);

    /** Issues CLOSE command */
    virtual void close (void);

    /** Sends a request and response */
    virtual void sendRequest (int connId, const char *id, const char *fname);
    virtual void sendResponse (int connId, const char *id, unsigned long size);

    /** When running under GUI, it displays the given string next to the icon */
    virtual void setStatusString (const char *s);
    //@}


    /** @name TCPSocket::CallbackInterface callback methods */

    //@{
    /** Does nothing but update statistics/status. Redefine to perform or schedule first sending. */
    virtual void socketEstablished(int connId, void *yourPtr);

    /**
     * Does nothing but update statistics/status. Redefine to perform or schedule next sending.
     * Beware: this funcion deletes the incoming message, which might not be what you want.
     */
    virtual void socketDataArrived(int connId, void *yourPtr, cPacket *msg, bool urgent);

    /** Since remote TCP closed, invokes close(). Redefine if you want to do something else. */
    virtual void socketPeerClosed(int connId, void *yourPtr);

    /** Does nothing but update statistics/status. Redefine if you want to do something else, such as opening a new connection. */
    virtual void socketClosed(int connId, void *yourPtr);

    /** Does nothing but update statistics/status. Redefine if you want to try reconnecting after a delay. */
    virtual void socketFailure(int connId, void *yourPtr, int code);

    /** Redefine to handle incoming TCPStatusInfo. */
    virtual void socketStatusArrived(int connId, void *yourPtr, TCPStatusInfo *status) {delete status;}
    //@}

};



#endif /* P2PAPP_H_ */
