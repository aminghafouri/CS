package ClientServerWireless;

@license(LGPL);


// we want our application to support the TCPApp interface
import inet.applications.ITCPApp;

// our module behaves like the TCPApp. At simulation time, we provide the appropriate value.
simple ClientApp like ITCPApp
{
	parameters:
	    string id = default ("client");              // some name assigned to me (for debugging purposes)
        string connectAddress = default ("serverApp");    // server address (may be symbolic)
        int connectPort = default (1000);        	   // port number of the server to connect to
        int fileSize @unit(byte) = default (1KiB);	   // size of file to be transferred
        int numAttempts = default (1);				   // number of times the file should be requested
        // different modes of transferring. I think bytecount is not realistic. But bytestream and object probably are.
        string dataTransferMode @enum ("bytecount","object","bytestream") = default ("object");
        @display ("i=block/app");
 		@statistic[eed](title="end-to-end delay of received packets";unit=s);
   		@statistic[jitter](title="jitter of received packets");
    gates:
        // since we are at the application layer, the only things we are interested in are how we
        // connect to the transport layer below us and how the transport layer talks to us.
        input tcpIn @labels(TCPCommand/up);
        output tcpOut @labels(TCPCommand/down);
}

// our module behaves like the TCPApp. At simulation time, we provide the appropriate value.
simple ServerApp like ITCPApp
{
	parameters:
        string localAddress = default ("serverApp"); // decides what is our addr. May be left empty ("") -- I assume
                                                  // this will mean "any interface"
        int localPort = default (1000);           // port number to listen on
        // different modes of transferring. I think bytecount is not realistic. But bytestream and object probably are.
        string dataTransferMode @enum ("bytecount","object","bytestream") = default ("object");
        
        @display ("i=block/app");
 
    gates:
        // since we are at the application layer, the only things we are interested in are how we
        // connect to the transport layer below us and how the transport layer talks to us.
        input tcpIn @labels(TCPCommand/up);
        output tcpOut @labels(TCPCommand/down);
}

