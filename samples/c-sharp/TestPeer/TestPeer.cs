using System;
using System.Threading;
using System.IO;
//using System.Collections;
using System.Collections.Generic;
using FlatBuffers;

class TestPeer
{
	const byte CHANNEL = 52;

	bool connected = false;
	DateTime last = DateTime.UtcNow;
	HumbleNet.PeerId myPeer = HumbleNet.PeerId.Invalid;
	List<HumbleNet.PeerId> pendingPeers = new List<HumbleNet.PeerId>();
	List<HumbleNet.PeerId> connectedPeers = new List<HumbleNet.PeerId>();

#region Message delivery
	VectorOffset buildKnownPeers(FlatBufferBuilder fbb)
	{
		var elems = pendingPeers.Count + connectedPeers.Count;
		TestClient.HelloPeer.StartPeersVector(fbb, elems);

		foreach(var peer in pendingPeers) {
			fbb.AddUint((UInt32)peer);
		}
		foreach(var peer in connectedPeers) {
			fbb.AddUint((UInt32)peer);
		}

		return fbb.EndVector();
	}

	byte[] getFlatBufferData(FlatBufferBuilder fbb)
	{
		byte[] buff;
		using (var ms = new MemoryStream(fbb.DataBuffer.Data, fbb.DataBuffer.Position, fbb.Offset))
		{
			buff = ms.ToArray();
		}
		return buff;
	}


	int sendMessage(HumbleNet.PeerId peer, byte[] data)
	{
		return HumbleNet.P2P.SendTo(data, peer, HumbleNet.SendMode.SEND_RELIABLE, CHANNEL);
	}

	void sendChat(string message)
	{
		var fbb = new FlatBufferBuilder(1);
		var chatmsg = fbb.CreateString(message);
		var chat = TestClient.Chat.CreateChat(fbb, chatmsg);
		var msg = TestClient.Message.CreateMessage(fbb, TestClient.MessageSwitch.Chat, chat.Value);
		TestClient.Message.FinishMessageBuffer(fbb, msg);

		byte[] data = getFlatBufferData(fbb);

		List<HumbleNet.PeerId> toRemove = null;

		foreach(var peer in connectedPeers) {
			if (sendMessage(peer, data) < 0) {
				if (toRemove == null) {
					toRemove = new List<HumbleNet.PeerId>();
				}
				toRemove.Insert(0, peer);
			}
		}

		if (toRemove != null) {
			foreach(var peer in toRemove) {
				disconnectPeer(peer);
			}
		}
	}

	bool sendHello(HumbleNet.PeerId peer)
	{
		Console.WriteLine("Try Hello {0}", peer);
		var fbb = new FlatBufferBuilder(1);

		var knownPeers = buildKnownPeers(fbb);
		var pos = TestClient.HelloPeer.CreateHelloPeer(fbb, knownPeers);
		var outmsg = TestClient.Message.CreateMessage(fbb, TestClient.MessageSwitch.HelloPeer, pos.Value);
		TestClient.Message.FinishMessageBuffer(fbb, outmsg);
		
		return sendMessage(peer, getFlatBufferData(fbb)) >= 0;
	}
#endregion

#region Peer Management
	void connectedToPeer(HumbleNet.PeerId peer)
	{
		pendingPeers.Remove(peer);
		if (!connectedPeers.Contains(peer)) {
			Console.WriteLine("Connected to Peer {0}", peer);
			connectedPeers.Insert(0, peer);
		}
	}
	
	void connectToPeer(HumbleNet.PeerId peer)
	{
		if (peer != myPeer && !pendingPeers.Contains(peer) && !connectedPeers.Contains(peer)) {
			Console.WriteLine("Connecting to Peer {0}", peer);
			pendingPeers.Insert(0, peer);
		}
	}

	void disconnectPeer(HumbleNet.PeerId peer)
	{
		Console.WriteLine("Disconnecting Peer {0}", peer);
		pendingPeers.Remove(peer);
		connectedPeers.Remove(peer);
	}
#endregion

#region Message processing
	void processKnownPeers(TestClient.HelloPeer message)
	{
		for (var i = 0; i < message.PeersLength; ++i) {
			HumbleNet.PeerId newPeer = (HumbleNet.PeerId)message.GetPeers(i);
			connectToPeer(newPeer);
		}
	}

	void processMessage(HumbleNet.PeerId peer, byte[] data)
	{
		var bb = new ByteBuffer(data);
		var msg = TestClient.Message.GetRootAsMessage(bb);
		switch (msg.MessageType) {
		case TestClient.MessageSwitch.HelloPeer: {
			var hello = (TestClient.HelloPeer)msg.GetMessage(new TestClient.HelloPeer());
			Console.WriteLine("Received hello from peer {0} response {1}", peer, hello.IsResponse);

			connectedToPeer(peer);

			processKnownPeers(hello);


			if (!hello.IsResponse) {
				var fbb = new FlatBufferBuilder(1);

				var knownPeers = buildKnownPeers(fbb);
				var pos = TestClient.HelloPeer.CreateHelloPeer(fbb, knownPeers, true);
				var outmsg = TestClient.Message.CreateMessage(fbb, TestClient.MessageSwitch.HelloPeer, pos.Value);
				TestClient.Message.FinishMessageBuffer(fbb, outmsg);

				sendMessage(peer, getFlatBufferData(fbb));
			}
		}
			break;
		case TestClient.MessageSwitch.Chat: {
			var chat = (TestClient.Chat)msg.GetMessage(new TestClient.Chat());
			Console.WriteLine("Peer {0}: {1}", peer, chat.Message);
		}
			break;
		default:
			Console.WriteLine("Unexpected message {0}", msg.MessageType);
			break;
		}
	}
#endregion

	volatile bool _running = true;
	Thread inputThread;

	void readInput()
	{
		while(_running)
		{
			string command = Console.ReadLine().ToLower();

			if (command == "/quit") {
				_running = false;
			} else if (command.StartsWith("/peer ")) {
				string peerStr = command.Substring(6);
				UInt32 peerId;
				if (UInt32.TryParse(peerStr, out peerId)) {
					connectToPeer((HumbleNet.PeerId)peerId);
				} else {
					connectToPeer(HumbleNet.P2P.VirtualPeerForAlias(peerStr));
				}
			} else if (command.StartsWith("/alias ")) {
				HumbleNet.P2P.RegisterAlias(command.Substring(7));
			} else if (command.StartsWith("/unalias ")) {
				HumbleNet.P2P.UnregisterAlias(command.Substring(9));
			} else if (command == "/unalias") {
				HumbleNet.P2P.UnregisterAllAliases();
			} else if (command.StartsWith("/disconnect ")) {
				string peerStr = command.Substring(12);
				UInt32 peerId;
				if (UInt32.TryParse(peerStr, out peerId)) {
					disconnectPeer((HumbleNet.PeerId)peerId);
				} else {
					disconnectPeer(HumbleNet.P2P.VirtualPeerForAlias(peerStr));
				}
			} else if (command.StartsWith("/")) {
				Console.WriteLine("Unknown command: {0}", command);
			} else {
				sendChat(command);
			}
		}
	}

	void Start()
	{
		inputThread = new Thread(readInput);
		inputThread.Start();
	}

	void Stop()
	{
		if(inputThread != null) {
			inputThread.Join();
		}
	}

	bool Run()
	{
		HumbleNet.P2P.Wait(100);

		if (myPeer.isInvalid()) {
			myPeer = HumbleNet.P2P.getMyPeerId();
			if (myPeer.isValid()) {
				Console.WriteLine("My Peer is {0}", myPeer);
				connected = true;
			}
		}

		if (connected) {
			List<HumbleNet.PeerId> toRemove = null;

			bool done = false;
			while (!done) {
				byte[] data;

				HumbleNet.PeerId otherPeer;
				if (HumbleNet.P2P.RecvFrom(out data, out otherPeer, CHANNEL) > 0) {
					processMessage(otherPeer, data);
				} else if (otherPeer.isValid()) {
					if(toRemove == null) { 
						toRemove = new List<HumbleNet.PeerId>();
					}
					toRemove.Insert(0, otherPeer);
				} else {
					done = true;
				}
			}

			DateTime now = DateTime.UtcNow;

			var span = now.Subtract(last);
			if (span.TotalSeconds > 1) {
				foreach (var peer in pendingPeers) {
					if (!sendHello(peer)) {
						if(toRemove == null) { 
							toRemove = new List<HumbleNet.PeerId>();
						}
						toRemove.Insert(0, peer);
					}
				}
				last = now;
			}

			// disconnect lost peers
			if (toRemove != null) {
				foreach(var peer in toRemove) {
					disconnectPeer(peer);
				}
			}
		}
		return _running;
	}

	public static void Main (string[] args)
	{
		string peerServer = "ws://localhost:8080/ws";

		if (args.Length == 1) {
			peerServer = args[0];
		}
		TestPeer app = new TestPeer();

		if (!HumbleNet.Init()) {
			Console.WriteLine("Failed to startup");
			return;
		}
		
		HumbleNet.P2P.Initialize(peerServer, "client_token", "client_secret", null);

		app.Start();

		while(app.Run()) {
		}

		app.Stop();

		HumbleNet.Shutdown();
	}
}
