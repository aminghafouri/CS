//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

enum CS_MSG_TYPE {
    CS_REQUEST = 0;
    CS_RESPONSE = 1;
};

packet CS_Packet
{
   int     type @enum(CS_MSG_TYPE);  // our message type
};

packet CS_Req extends CS_Packet
{
    string	id;		        // announce the requestor's ID
    int filesize;	        // request for a file
};

packet CS_Resp extends CS_Packet
{
    string  id;			// ID of the responder
    char data [];	    // contents of the file
};
