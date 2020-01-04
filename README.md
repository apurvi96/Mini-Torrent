# <h2>Mini-Torrent</h2>
A peer to peer(P2P) file tansfer protocol to share data over the network where the peers can
share or download files amongst the members belonging to same group.
<ul>
<li>Synchronized trackers : Maintain metadata about the peers along with their group details,files shared, port and IP addresses.</li>
<li>Parallel Downloading: Support for fast download with multiple pieces coming and downloaded from multiple clients simultaneously.</li>
<li>Various modules for client such as create groups, list requests, stop sharing , Show downloads, logout etc.</li>
</ul>
<b>Key Area</b>: C++, PThread, Socket programming,Multi Threading

<h2>Architecture Overview</h2>
The system consists of following entities:<br>
SYNCHRONIZED TRACKERS:<ul>
<li> Maintain information of clients with their files(shared by client) to assist the clients
for the communication between peers</li>
<li>Trackers should be synchronized i.e all the trackers if online should be in sync
with each other</li></ul>
CLIENTS:<br/>
<ul>
<li>Client creates an account, register with tracker and login using the user credentials.</li>
<li>Client can create groups, fetch list of all groups,request to join groups, leave groups, accept join requests(if owner).</li>
<li>Share file across group: Share the filename and SHA1 hash of the complete file
as well as piecewise SHA1 with the tracker.</li>
<li>Download file:<br/>1.Retrieve peer information from tracker for the file.<br/>2.Download file from multiple peers (different pieces of file from
different peers - <b>piece selection algorithm)</b>  simultaneously and all the
files which client downloads will be shareable to other users in the same
group.</li>
<b><i>Ensured file integrity from SHA1 comparison</b></i>
<li>Stop sharing all files(Logout)</li>
<li>Whenever client logins, all previously shared files before logout 
automatically goes on sharing mode.</li>
</ul>
<h2>Compile and Run Code</h2>
<ul>
<li> g++ -o client client.cpp -lpthread -lcrypto<br>./client</li>
<li>g++ -o tracker tracker.cpp -lpthread -lcrypto<br>./tracker</li>
</ul>




