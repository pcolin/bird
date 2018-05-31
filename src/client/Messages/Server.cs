// <auto-generated>
//     Generated by the protocol buffer compiler.  DO NOT EDIT!
//     source: Server.proto
// </auto-generated>
#pragma warning disable 1591, 0612, 3021
#region Designer generated code

using pb = global::Google.Protobuf;
using pbc = global::Google.Protobuf.Collections;
using pbr = global::Google.Protobuf.Reflection;
using scg = global::System.Collections.Generic;
namespace Proto {

  /// <summary>Holder for reflection information generated from Server.proto</summary>
  public static partial class ServerReflection {

    #region Descriptor
    /// <summary>File descriptor for Server.proto</summary>
    public static pbr::FileDescriptor Descriptor {
      get { return descriptor; }
    }
    private static pbr::FileDescriptor descriptor;

    static ServerReflection() {
      byte[] descriptorData = global::System.Convert.FromBase64String(
          string.Concat(
            "CgxTZXJ2ZXIucHJvdG8SBVByb3RvGg5FeGNoYW5nZS5wcm90byKUAQoKU2Vy",
            "dmVySW5mbxIhCghleGNoYW5nZRgBIAEoDjIPLlByb3RvLkV4Y2hhbmdlEgwK",
            "BGluZm8YAiABKAkSJAoEdHlwZRgDIAEoDjIWLlByb3RvLlNlcnZlckluZm8u",
            "VHlwZRIMCgR0aW1lGAQgASgEIiEKBFR5cGUSBwoDUFVCEAASBwoDV0FOEAES",
            "BwoDRVJSEAJiBnByb3RvMw=="));
      descriptor = pbr::FileDescriptor.FromGeneratedCode(descriptorData,
          new pbr::FileDescriptor[] { global::Proto.ExchangeReflection.Descriptor, },
          new pbr::GeneratedClrTypeInfo(null, new pbr::GeneratedClrTypeInfo[] {
            new pbr::GeneratedClrTypeInfo(typeof(global::Proto.ServerInfo), global::Proto.ServerInfo.Parser, new[]{ "Exchange", "Info", "Type", "Time" }, null, new[]{ typeof(global::Proto.ServerInfo.Types.Type) }, null)
          }));
    }
    #endregion

  }
  #region Messages
  public sealed partial class ServerInfo : pb::IMessage<ServerInfo> {
    private static readonly pb::MessageParser<ServerInfo> _parser = new pb::MessageParser<ServerInfo>(() => new ServerInfo());
    private pb::UnknownFieldSet _unknownFields;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public static pb::MessageParser<ServerInfo> Parser { get { return _parser; } }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public static pbr::MessageDescriptor Descriptor {
      get { return global::Proto.ServerReflection.Descriptor.MessageTypes[0]; }
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    pbr::MessageDescriptor pb::IMessage.Descriptor {
      get { return Descriptor; }
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public ServerInfo() {
      OnConstruction();
    }

    partial void OnConstruction();

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public ServerInfo(ServerInfo other) : this() {
      exchange_ = other.exchange_;
      info_ = other.info_;
      type_ = other.type_;
      time_ = other.time_;
      _unknownFields = pb::UnknownFieldSet.Clone(other._unknownFields);
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public ServerInfo Clone() {
      return new ServerInfo(this);
    }

    /// <summary>Field number for the "exchange" field.</summary>
    public const int ExchangeFieldNumber = 1;
    private global::Proto.Exchange exchange_ = 0;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public global::Proto.Exchange Exchange {
      get { return exchange_; }
      set {
        exchange_ = value;
      }
    }

    /// <summary>Field number for the "info" field.</summary>
    public const int InfoFieldNumber = 2;
    private string info_ = "";
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public string Info {
      get { return info_; }
      set {
        info_ = pb::ProtoPreconditions.CheckNotNull(value, "value");
      }
    }

    /// <summary>Field number for the "type" field.</summary>
    public const int TypeFieldNumber = 3;
    private global::Proto.ServerInfo.Types.Type type_ = 0;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public global::Proto.ServerInfo.Types.Type Type {
      get { return type_; }
      set {
        type_ = value;
      }
    }

    /// <summary>Field number for the "time" field.</summary>
    public const int TimeFieldNumber = 4;
    private ulong time_;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public ulong Time {
      get { return time_; }
      set {
        time_ = value;
      }
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public override bool Equals(object other) {
      return Equals(other as ServerInfo);
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public bool Equals(ServerInfo other) {
      if (ReferenceEquals(other, null)) {
        return false;
      }
      if (ReferenceEquals(other, this)) {
        return true;
      }
      if (Exchange != other.Exchange) return false;
      if (Info != other.Info) return false;
      if (Type != other.Type) return false;
      if (Time != other.Time) return false;
      return Equals(_unknownFields, other._unknownFields);
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public override int GetHashCode() {
      int hash = 1;
      if (Exchange != 0) hash ^= Exchange.GetHashCode();
      if (Info.Length != 0) hash ^= Info.GetHashCode();
      if (Type != 0) hash ^= Type.GetHashCode();
      if (Time != 0UL) hash ^= Time.GetHashCode();
      if (_unknownFields != null) {
        hash ^= _unknownFields.GetHashCode();
      }
      return hash;
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public override string ToString() {
      return pb::JsonFormatter.ToDiagnosticString(this);
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public void WriteTo(pb::CodedOutputStream output) {
      if (Exchange != 0) {
        output.WriteRawTag(8);
        output.WriteEnum((int) Exchange);
      }
      if (Info.Length != 0) {
        output.WriteRawTag(18);
        output.WriteString(Info);
      }
      if (Type != 0) {
        output.WriteRawTag(24);
        output.WriteEnum((int) Type);
      }
      if (Time != 0UL) {
        output.WriteRawTag(32);
        output.WriteUInt64(Time);
      }
      if (_unknownFields != null) {
        _unknownFields.WriteTo(output);
      }
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public int CalculateSize() {
      int size = 0;
      if (Exchange != 0) {
        size += 1 + pb::CodedOutputStream.ComputeEnumSize((int) Exchange);
      }
      if (Info.Length != 0) {
        size += 1 + pb::CodedOutputStream.ComputeStringSize(Info);
      }
      if (Type != 0) {
        size += 1 + pb::CodedOutputStream.ComputeEnumSize((int) Type);
      }
      if (Time != 0UL) {
        size += 1 + pb::CodedOutputStream.ComputeUInt64Size(Time);
      }
      if (_unknownFields != null) {
        size += _unknownFields.CalculateSize();
      }
      return size;
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public void MergeFrom(ServerInfo other) {
      if (other == null) {
        return;
      }
      if (other.Exchange != 0) {
        Exchange = other.Exchange;
      }
      if (other.Info.Length != 0) {
        Info = other.Info;
      }
      if (other.Type != 0) {
        Type = other.Type;
      }
      if (other.Time != 0UL) {
        Time = other.Time;
      }
      _unknownFields = pb::UnknownFieldSet.MergeFrom(_unknownFields, other._unknownFields);
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public void MergeFrom(pb::CodedInputStream input) {
      uint tag;
      while ((tag = input.ReadTag()) != 0) {
        switch(tag) {
          default:
            _unknownFields = pb::UnknownFieldSet.MergeFieldFrom(_unknownFields, input);
            break;
          case 8: {
            exchange_ = (global::Proto.Exchange) input.ReadEnum();
            break;
          }
          case 18: {
            Info = input.ReadString();
            break;
          }
          case 24: {
            type_ = (global::Proto.ServerInfo.Types.Type) input.ReadEnum();
            break;
          }
          case 32: {
            Time = input.ReadUInt64();
            break;
          }
        }
      }
    }

    #region Nested types
    /// <summary>Container for nested types declared in the ServerInfo message type.</summary>
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public static partial class Types {
      public enum Type {
        [pbr::OriginalName("PUB")] Pub = 0,
        [pbr::OriginalName("WAN")] Wan = 1,
        [pbr::OriginalName("ERR")] Err = 2,
      }

    }
    #endregion

  }

  #endregion

}

#endregion Designer generated code
