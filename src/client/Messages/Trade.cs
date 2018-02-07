// <auto-generated>
//     Generated by the protocol buffer compiler.  DO NOT EDIT!
//     source: Trade.proto
// </auto-generated>
#pragma warning disable 1591, 0612, 3021
#region Designer generated code

using pb = global::Google.Protobuf;
using pbc = global::Google.Protobuf.Collections;
using pbr = global::Google.Protobuf.Reflection;
using scg = global::System.Collections.Generic;
namespace Proto {

  /// <summary>Holder for reflection information generated from Trade.proto</summary>
  public static partial class TradeReflection {

    #region Descriptor
    /// <summary>File descriptor for Trade.proto</summary>
    public static pbr::FileDescriptor Descriptor {
      get { return descriptor; }
    }
    private static pbr::FileDescriptor descriptor;

    static TradeReflection() {
      byte[] descriptorData = global::System.Convert.FromBase64String(
          string.Concat(
            "CgtUcmFkZS5wcm90bxIFcHJvdG8aC09yZGVyLnByb3RvIoEBCgVUcmFkZRIK",
            "CgJpZBgBIAEoCRISCgppbnN0cnVtZW50GAIgASgJEhkKBHNpZGUYAyABKA4y",
            "Cy5wcm90by5TaWRlEg0KBXByaWNlGAQgASgBEg4KBnZvbHVtZRgFIAEoBRIM",
            "CgR0aW1lGAYgASgEEhAKCG9yZGVyX2lkGAcgASgEYgZwcm90bzM="));
      descriptor = pbr::FileDescriptor.FromGeneratedCode(descriptorData,
          new pbr::FileDescriptor[] { global::Proto.OrderReflection.Descriptor, },
          new pbr::GeneratedClrTypeInfo(null, new pbr::GeneratedClrTypeInfo[] {
            new pbr::GeneratedClrTypeInfo(typeof(global::Proto.Trade), global::Proto.Trade.Parser, new[]{ "Id", "Instrument", "Side", "Price", "Volume", "Time", "OrderId" }, null, null, null)
          }));
    }
    #endregion

  }
  #region Messages
  public sealed partial class Trade : pb::IMessage<Trade> {
    private static readonly pb::MessageParser<Trade> _parser = new pb::MessageParser<Trade>(() => new Trade());
    private pb::UnknownFieldSet _unknownFields;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public static pb::MessageParser<Trade> Parser { get { return _parser; } }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public static pbr::MessageDescriptor Descriptor {
      get { return global::Proto.TradeReflection.Descriptor.MessageTypes[0]; }
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    pbr::MessageDescriptor pb::IMessage.Descriptor {
      get { return Descriptor; }
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public Trade() {
      OnConstruction();
    }

    partial void OnConstruction();

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public Trade(Trade other) : this() {
      id_ = other.id_;
      instrument_ = other.instrument_;
      side_ = other.side_;
      price_ = other.price_;
      volume_ = other.volume_;
      time_ = other.time_;
      orderId_ = other.orderId_;
      _unknownFields = pb::UnknownFieldSet.Clone(other._unknownFields);
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public Trade Clone() {
      return new Trade(this);
    }

    /// <summary>Field number for the "id" field.</summary>
    public const int IdFieldNumber = 1;
    private string id_ = "";
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public string Id {
      get { return id_; }
      set {
        id_ = pb::ProtoPreconditions.CheckNotNull(value, "value");
      }
    }

    /// <summary>Field number for the "instrument" field.</summary>
    public const int InstrumentFieldNumber = 2;
    private string instrument_ = "";
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public string Instrument {
      get { return instrument_; }
      set {
        instrument_ = pb::ProtoPreconditions.CheckNotNull(value, "value");
      }
    }

    /// <summary>Field number for the "side" field.</summary>
    public const int SideFieldNumber = 3;
    private global::Proto.Side side_ = 0;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public global::Proto.Side Side {
      get { return side_; }
      set {
        side_ = value;
      }
    }

    /// <summary>Field number for the "price" field.</summary>
    public const int PriceFieldNumber = 4;
    private double price_;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public double Price {
      get { return price_; }
      set {
        price_ = value;
      }
    }

    /// <summary>Field number for the "volume" field.</summary>
    public const int VolumeFieldNumber = 5;
    private int volume_;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public int Volume {
      get { return volume_; }
      set {
        volume_ = value;
      }
    }

    /// <summary>Field number for the "time" field.</summary>
    public const int TimeFieldNumber = 6;
    private ulong time_;
    /// <summary>
    ///google.protobuf.Timestamp time = 6;
    /// </summary>
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public ulong Time {
      get { return time_; }
      set {
        time_ = value;
      }
    }

    /// <summary>Field number for the "order_id" field.</summary>
    public const int OrderIdFieldNumber = 7;
    private ulong orderId_;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public ulong OrderId {
      get { return orderId_; }
      set {
        orderId_ = value;
      }
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public override bool Equals(object other) {
      return Equals(other as Trade);
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public bool Equals(Trade other) {
      if (ReferenceEquals(other, null)) {
        return false;
      }
      if (ReferenceEquals(other, this)) {
        return true;
      }
      if (Id != other.Id) return false;
      if (Instrument != other.Instrument) return false;
      if (Side != other.Side) return false;
      if (!pbc::ProtobufEqualityComparers.BitwiseDoubleEqualityComparer.Equals(Price, other.Price)) return false;
      if (Volume != other.Volume) return false;
      if (Time != other.Time) return false;
      if (OrderId != other.OrderId) return false;
      return Equals(_unknownFields, other._unknownFields);
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public override int GetHashCode() {
      int hash = 1;
      if (Id.Length != 0) hash ^= Id.GetHashCode();
      if (Instrument.Length != 0) hash ^= Instrument.GetHashCode();
      if (Side != 0) hash ^= Side.GetHashCode();
      if (Price != 0D) hash ^= pbc::ProtobufEqualityComparers.BitwiseDoubleEqualityComparer.GetHashCode(Price);
      if (Volume != 0) hash ^= Volume.GetHashCode();
      if (Time != 0UL) hash ^= Time.GetHashCode();
      if (OrderId != 0UL) hash ^= OrderId.GetHashCode();
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
      if (Id.Length != 0) {
        output.WriteRawTag(10);
        output.WriteString(Id);
      }
      if (Instrument.Length != 0) {
        output.WriteRawTag(18);
        output.WriteString(Instrument);
      }
      if (Side != 0) {
        output.WriteRawTag(24);
        output.WriteEnum((int) Side);
      }
      if (Price != 0D) {
        output.WriteRawTag(33);
        output.WriteDouble(Price);
      }
      if (Volume != 0) {
        output.WriteRawTag(40);
        output.WriteInt32(Volume);
      }
      if (Time != 0UL) {
        output.WriteRawTag(48);
        output.WriteUInt64(Time);
      }
      if (OrderId != 0UL) {
        output.WriteRawTag(56);
        output.WriteUInt64(OrderId);
      }
      if (_unknownFields != null) {
        _unknownFields.WriteTo(output);
      }
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public int CalculateSize() {
      int size = 0;
      if (Id.Length != 0) {
        size += 1 + pb::CodedOutputStream.ComputeStringSize(Id);
      }
      if (Instrument.Length != 0) {
        size += 1 + pb::CodedOutputStream.ComputeStringSize(Instrument);
      }
      if (Side != 0) {
        size += 1 + pb::CodedOutputStream.ComputeEnumSize((int) Side);
      }
      if (Price != 0D) {
        size += 1 + 8;
      }
      if (Volume != 0) {
        size += 1 + pb::CodedOutputStream.ComputeInt32Size(Volume);
      }
      if (Time != 0UL) {
        size += 1 + pb::CodedOutputStream.ComputeUInt64Size(Time);
      }
      if (OrderId != 0UL) {
        size += 1 + pb::CodedOutputStream.ComputeUInt64Size(OrderId);
      }
      if (_unknownFields != null) {
        size += _unknownFields.CalculateSize();
      }
      return size;
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public void MergeFrom(Trade other) {
      if (other == null) {
        return;
      }
      if (other.Id.Length != 0) {
        Id = other.Id;
      }
      if (other.Instrument.Length != 0) {
        Instrument = other.Instrument;
      }
      if (other.Side != 0) {
        Side = other.Side;
      }
      if (other.Price != 0D) {
        Price = other.Price;
      }
      if (other.Volume != 0) {
        Volume = other.Volume;
      }
      if (other.Time != 0UL) {
        Time = other.Time;
      }
      if (other.OrderId != 0UL) {
        OrderId = other.OrderId;
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
          case 10: {
            Id = input.ReadString();
            break;
          }
          case 18: {
            Instrument = input.ReadString();
            break;
          }
          case 24: {
            side_ = (global::Proto.Side) input.ReadEnum();
            break;
          }
          case 33: {
            Price = input.ReadDouble();
            break;
          }
          case 40: {
            Volume = input.ReadInt32();
            break;
          }
          case 48: {
            Time = input.ReadUInt64();
            break;
          }
          case 56: {
            OrderId = input.ReadUInt64();
            break;
          }
        }
      }
    }

  }

  #endregion

}

#endregion Designer generated code
