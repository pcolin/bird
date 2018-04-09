// <auto-generated>
//     Generated by the protocol buffer compiler.  DO NOT EDIT!
//     source: Heartbeat.proto
// </auto-generated>
#pragma warning disable 1591, 0612, 3021
#region Designer generated code

using pb = global::Google.Protobuf;
using pbc = global::Google.Protobuf.Collections;
using pbr = global::Google.Protobuf.Reflection;
using scg = global::System.Collections.Generic;
namespace Proto {

  /// <summary>Holder for reflection information generated from Heartbeat.proto</summary>
  public static partial class HeartbeatReflection {

    #region Descriptor
    /// <summary>File descriptor for Heartbeat.proto</summary>
    public static pbr::FileDescriptor Descriptor {
      get { return descriptor; }
    }
    private static pbr::FileDescriptor descriptor;

    static HeartbeatReflection() {
      byte[] descriptorData = global::System.Convert.FromBase64String(
          string.Concat(
            "Cg9IZWFydGJlYXQucHJvdG8SBVByb3RvIksKCUhlYXJ0YmVhdBIiCgR0eXBl",
            "GAEgASgOMhQuUHJvdG8uUHJvY2Vzc29yVHlwZRIMCgRuYW1lGAIgASgJEgwK",
            "BHRpbWUYAyABKAQqUAoNUHJvY2Vzc29yVHlwZRIOCgpNaWRkbGV3YXJlEAAS",
            "CwoHTW9uaXRvchABEgsKB1ByaWNpbmcQAhIMCghTdHJhdGVneRADEgcKA0dV",
            "SRAEYgZwcm90bzM="));
      descriptor = pbr::FileDescriptor.FromGeneratedCode(descriptorData,
          new pbr::FileDescriptor[] { },
          new pbr::GeneratedClrTypeInfo(new[] {typeof(global::Proto.ProcessorType), }, new pbr::GeneratedClrTypeInfo[] {
            new pbr::GeneratedClrTypeInfo(typeof(global::Proto.Heartbeat), global::Proto.Heartbeat.Parser, new[]{ "Type", "Name", "Time" }, null, null, null)
          }));
    }
    #endregion

  }
  #region Enums
  public enum ProcessorType {
    [pbr::OriginalName("Middleware")] Middleware = 0,
    [pbr::OriginalName("Monitor")] Monitor = 1,
    [pbr::OriginalName("Pricing")] Pricing = 2,
    [pbr::OriginalName("Strategy")] Strategy = 3,
    [pbr::OriginalName("GUI")] Gui = 4,
  }

  #endregion

  #region Messages
  public sealed partial class Heartbeat : pb::IMessage<Heartbeat> {
    private static readonly pb::MessageParser<Heartbeat> _parser = new pb::MessageParser<Heartbeat>(() => new Heartbeat());
    private pb::UnknownFieldSet _unknownFields;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public static pb::MessageParser<Heartbeat> Parser { get { return _parser; } }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public static pbr::MessageDescriptor Descriptor {
      get { return global::Proto.HeartbeatReflection.Descriptor.MessageTypes[0]; }
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    pbr::MessageDescriptor pb::IMessage.Descriptor {
      get { return Descriptor; }
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public Heartbeat() {
      OnConstruction();
    }

    partial void OnConstruction();

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public Heartbeat(Heartbeat other) : this() {
      type_ = other.type_;
      name_ = other.name_;
      time_ = other.time_;
      _unknownFields = pb::UnknownFieldSet.Clone(other._unknownFields);
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public Heartbeat Clone() {
      return new Heartbeat(this);
    }

    /// <summary>Field number for the "type" field.</summary>
    public const int TypeFieldNumber = 1;
    private global::Proto.ProcessorType type_ = 0;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public global::Proto.ProcessorType Type {
      get { return type_; }
      set {
        type_ = value;
      }
    }

    /// <summary>Field number for the "name" field.</summary>
    public const int NameFieldNumber = 2;
    private string name_ = "";
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public string Name {
      get { return name_; }
      set {
        name_ = pb::ProtoPreconditions.CheckNotNull(value, "value");
      }
    }

    /// <summary>Field number for the "time" field.</summary>
    public const int TimeFieldNumber = 3;
    private ulong time_;
    /// <summary>
    /// google.protobuf.Timestamp time = 2;
    /// </summary>
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public ulong Time {
      get { return time_; }
      set {
        time_ = value;
      }
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public override bool Equals(object other) {
      return Equals(other as Heartbeat);
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public bool Equals(Heartbeat other) {
      if (ReferenceEquals(other, null)) {
        return false;
      }
      if (ReferenceEquals(other, this)) {
        return true;
      }
      if (Type != other.Type) return false;
      if (Name != other.Name) return false;
      if (Time != other.Time) return false;
      return Equals(_unknownFields, other._unknownFields);
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public override int GetHashCode() {
      int hash = 1;
      if (Type != 0) hash ^= Type.GetHashCode();
      if (Name.Length != 0) hash ^= Name.GetHashCode();
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
      if (Type != 0) {
        output.WriteRawTag(8);
        output.WriteEnum((int) Type);
      }
      if (Name.Length != 0) {
        output.WriteRawTag(18);
        output.WriteString(Name);
      }
      if (Time != 0UL) {
        output.WriteRawTag(24);
        output.WriteUInt64(Time);
      }
      if (_unknownFields != null) {
        _unknownFields.WriteTo(output);
      }
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public int CalculateSize() {
      int size = 0;
      if (Type != 0) {
        size += 1 + pb::CodedOutputStream.ComputeEnumSize((int) Type);
      }
      if (Name.Length != 0) {
        size += 1 + pb::CodedOutputStream.ComputeStringSize(Name);
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
    public void MergeFrom(Heartbeat other) {
      if (other == null) {
        return;
      }
      if (other.Type != 0) {
        Type = other.Type;
      }
      if (other.Name.Length != 0) {
        Name = other.Name;
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
            type_ = (global::Proto.ProcessorType) input.ReadEnum();
            break;
          }
          case 18: {
            Name = input.ReadString();
            break;
          }
          case 24: {
            Time = input.ReadUInt64();
            break;
          }
        }
      }
    }

  }

  #endregion

}

#endregion Designer generated code
