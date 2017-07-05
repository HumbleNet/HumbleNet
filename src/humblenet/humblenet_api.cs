using System;
using System.Runtime.InteropServices;

public static class HumbleNet {
#region Enums
	@enums@
#endregion

#region Typedefs
	public struct PeerId : System.IEquatable<PeerId>, System.IComparable<PeerId> {
		public static readonly PeerId Invalid = new PeerId(0);
		public UInt32 m_data;

		public PeerId(UInt32 value) {
			m_data = value;
		}

		public bool isValid() {
			return !isInvalid();
		}

		public bool isInvalid() {
			return m_data == Invalid.m_data;
		}

		public override string ToString() {
			return m_data.ToString();
		}

		public override bool Equals(object other) {
			return other is PeerId && this == (PeerId)other;
		}

		public override int GetHashCode() {
			return m_data.GetHashCode();
		}

		public static bool operator ==(PeerId x, PeerId y) {
			return x.m_data == y.m_data;
		}

		public static bool operator !=(PeerId x, PeerId y) {
			return !(x == y);
		}

		public static explicit operator PeerId(UInt32 value) {
			return new PeerId(value);
		}

		public static explicit operator UInt32(PeerId that) {
			return that.m_data;
		}

		public bool Equals(PeerId other) {
			return m_data == other.m_data;
		}

		public int CompareTo(PeerId other) {
			return m_data.CompareTo(other.m_data);
		}
	}

	// Miscellaneous C# Mappings

#endregion
#region Functions
	internal static class NativeMethods {
#if UNITY_EDITOR
		internal const string NativeLibraryName = "humblenet_unity_editor";
#elif UNITY_WEBGL
		internal const string NativeLibraryName = "__Internal";
#else
		internal const string NativeLibraryName = "humblenet";
#endif
		@functions:-native@
	}
#endregion
#region Public Interface
	public static UInt32 Version()
	{
		return NativeMethods.humblenet_version();
	}

	public static bool Init()
	{
		return NativeMethods.humblenet_init();
	}

	public static void Shutdown()
	{
		NativeMethods.humblenet_shutdown();
	}

	public static string getError()
	{
		return NativeMethods.humblenet_get_error();
	}

	public static void clearError()
	{
		NativeMethods.humblenet_clear_error();
	}

	public static bool setHint( string name, string value )
	{
		return NativeMethods.humblenet_set_hint( name, value );
	}

	public static string getHint( string name )
	{
		return NativeMethods.humblenet_get_hint( name );
	}

#region P2P API
	public static class P2P
	{
		public static bool isSupported()
		{
			return NativeMethods.humblenet_p2p_supported();
		}

		public static bool isInitialized()
		{
			return NativeMethods.humblenet_p2p_is_initialized();
		}

		public static bool Initialize(string server, string client_token, string client_secret, string auth_token)
		{
			return NativeMethods.humblenet_p2p_init(server, client_token, client_secret, auth_token);
		}

		public static PeerId getMyPeerId()
		{
			return (PeerId)NativeMethods.humblenet_p2p_get_my_peer_id();
		}

		public static bool RegisterAlias(string name)
		{
			return NativeMethods.humblenet_p2p_register_alias(name);
		}

		public static bool UnregisterAllAliases()
		{
			return NativeMethods.humblenet_p2p_unregister_alias(null);
		}

		public static bool UnregisterAlias(string name)
		{
			return NativeMethods.humblenet_p2p_unregister_alias(name);
		}

		public static PeerId VirtualPeerForAlias(string name)
		{
			return (PeerId)NativeMethods.humblenet_p2p_virtual_peer_for_alias(name);
		}

		public static int SendTo(byte[] message, PeerId toPeer, SendMode mode, byte channel)
		{
			return NativeMethods.humblenet_p2p_sendto(message, (uint)message.Length, (UInt32)toPeer, mode, channel);
		}

		public static int SendTo(byte[] message, uint length, PeerId toPeer, SendMode mode, byte channel)
		{
			return NativeMethods.humblenet_p2p_sendto(message, length, (UInt32)toPeer, mode, channel);
		}

		public static int SendTo(byte[] message, uint offset, uint length, PeerId toPeer, SendMode mode, byte channel)
		{
			byte[] buff;
			if (offset == 0) {
				buff = message;
			} else {
				buff = new byte[length];
				Buffer.BlockCopy(message, (int)offset, buff, 0, (int)length);
			}
			return NativeMethods.humblenet_p2p_sendto(buff, length, (UInt32)toPeer, mode, channel);
		}

		public static int RecvFrom(byte[] message, out PeerId fromPeer, byte channel)
		{
			UInt32 peer;
			int ret = NativeMethods.humblenet_p2p_recvfrom(message, (uint)message.Length, out peer, channel);
			fromPeer = (PeerId)peer;
			return ret;
		}

		public static int RecvFrom(out byte[] data, out PeerId fromPeer, byte channel)
		{
			UInt32 bufsize;
			UInt32 peer;

			if (! NativeMethods.humblenet_p2p_peek(out bufsize, channel) ) {
				data = null;
				fromPeer = PeerId.Invalid;
				return 0;
			}
			data = new byte[bufsize];

			int read = NativeMethods.humblenet_p2p_recvfrom(data, bufsize, out peer, channel);
			fromPeer = (PeerId)peer;
			if (read < 0) {
				data = null;
			}
			return read;
		}

		public static bool Peek(out UInt32 size, byte channel)
		{
			return NativeMethods.humblenet_p2p_peek(out size, channel);
		}

		public static bool DisconnectPeer(PeerId peer)
		{
			return NativeMethods.humblenet_p2p_disconnect((UInt32)peer);
		}

		public static bool Wait(int ms)
		{
			return NativeMethods.humblenet_p2p_wait(ms);
		}

	}
#endregion

#endregion
}
