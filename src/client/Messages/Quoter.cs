// <auto-generated>
//     Generated by the protocol buffer compiler.  DO NOT EDIT!
//     source: Quoter.proto
// </auto-generated>
#pragma warning disable 1591, 0612, 3021
#region Designer generated code

using pb = global::Google.Protobuf;
using pbc = global::Google.Protobuf.Collections;
using pbr = global::Google.Protobuf.Reflection;
using scg = global::System.Collections.Generic;
namespace Proto {

  /// <summary>Holder for reflection information generated from Quoter.proto</summary>
  public static partial class QuoterReflection {

    #region Descriptor
    /// <summary>File descriptor for Quoter.proto</summary>
    public static pbr::FileDescriptor Descriptor {
      get { return descriptor; }
    }
    private static pbr::FileDescriptor descriptor;

    static QuoterReflection() {
      byte[] descriptorData = global::System.Convert.FromBase64String(
          string.Concat(
            "CgxRdW90ZXIucHJvdG8SBVByb3RvGg5FeGNoYW5nZS5wcm90bxoNUmVxdWVz",
            "dC5wcm90bxoLUmVwbHkucHJvdG8inQIKClF1b3RlclNwZWMSDAoEbmFtZRgB",
            "IAEoCRIOCgZwcmljZXIYAiABKAkSEgoKdW5kZXJseWluZxgDIAEoCRITCgtk",
            "ZWx0YV9saW1pdBgEIAEoARITCgtvcmRlcl9saW1pdBgFIAEoBRITCgt0cmFk",
            "ZV9saW1pdBgGIAEoBRISCgpiaWRfdm9sdW1lGAcgASgFEhIKCmFza192b2x1",
            "bWUYCCABKAUSFwoPcmVzcG9uc2Vfdm9sdW1lGAkgASgFEg0KBWRlcHRoGAog",
            "ASgFEhQKDHJlZmlsbF90aW1lcxgLIAEoBRITCgt3aWRlX3NwcmVhZBgMIAEo",
            "CBISCgpwcm90ZWN0aW9uGA0gASgIEg8KB29wdGlvbnMYDiADKAkiggEKCVF1",
            "b3RlclJlcRIgCgR0eXBlGAEgASgOMhIuUHJvdG8uUmVxdWVzdFR5cGUSIQoI",
            "ZXhjaGFuZ2UYAiABKA4yDy5Qcm90by5FeGNoYW5nZRIMCgR1c2VyGAMgASgJ",
            "EiIKB3F1b3RlcnMYBCADKAsyES5Qcm90by5RdW90ZXJTcGVjIk0KCVF1b3Rl",
            "clJlcBIiCgdxdW90ZXJzGAEgAygLMhEuUHJvdG8uUXVvdGVyU3BlYxIcCgZy",
            "ZXN1bHQYAiABKAsyDC5Qcm90by5SZXBseWIGcHJvdG8z"));
      descriptor = pbr::FileDescriptor.FromGeneratedCode(descriptorData,
          new pbr::FileDescriptor[] { global::Proto.ExchangeReflection.Descriptor, global::Proto.RequestReflection.Descriptor, global::Proto.ReplyReflection.Descriptor, },
          new pbr::GeneratedClrTypeInfo(null, new pbr::GeneratedClrTypeInfo[] {
            new pbr::GeneratedClrTypeInfo(typeof(global::Proto.QuoterSpec), global::Proto.QuoterSpec.Parser, new[]{ "Name", "Pricer", "Underlying", "DeltaLimit", "OrderLimit", "TradeLimit", "BidVolume", "AskVolume", "ResponseVolume", "Depth", "RefillTimes", "WideSpread", "Protection", "Options" }, null, null, null),
            new pbr::GeneratedClrTypeInfo(typeof(global::Proto.QuoterReq), global::Proto.QuoterReq.Parser, new[]{ "Type", "Exchange", "User", "Quoters" }, null, null, null),
            new pbr::GeneratedClrTypeInfo(typeof(global::Proto.QuoterRep), global::Proto.QuoterRep.Parser, new[]{ "Quoters", "Result" }, null, null, null)
          }));
    }
    #endregion

  }
  #region Messages
  public sealed partial class QuoterSpec : pb::IMessage<QuoterSpec> {
    private static readonly pb::MessageParser<QuoterSpec> _parser = new pb::MessageParser<QuoterSpec>(() => new QuoterSpec());
    private pb::UnknownFieldSet _unknownFields;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public static pb::MessageParser<QuoterSpec> Parser { get { return _parser; } }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public static pbr::MessageDescriptor Descriptor {
      get { return global::Proto.QuoterReflection.Descriptor.MessageTypes[0]; }
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    pbr::MessageDescriptor pb::IMessage.Descriptor {
      get { return Descriptor; }
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public QuoterSpec() {
      OnConstruction();
    }

    partial void OnConstruction();

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public QuoterSpec(QuoterSpec other) : this() {
      name_ = other.name_;
      pricer_ = other.pricer_;
      underlying_ = other.underlying_;
      deltaLimit_ = other.deltaLimit_;
      orderLimit_ = other.orderLimit_;
      tradeLimit_ = other.tradeLimit_;
      bidVolume_ = other.bidVolume_;
      askVolume_ = other.askVolume_;
      responseVolume_ = other.responseVolume_;
      depth_ = other.depth_;
      refillTimes_ = other.refillTimes_;
      wideSpread_ = other.wideSpread_;
      protection_ = other.protection_;
      options_ = other.options_.Clone();
      _unknownFields = pb::UnknownFieldSet.Clone(other._unknownFields);
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public QuoterSpec Clone() {
      return new QuoterSpec(this);
    }

    /// <summary>Field number for the "name" field.</summary>
    public const int NameFieldNumber = 1;
    private string name_ = "";
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public string Name {
      get { return name_; }
      set {
        name_ = pb::ProtoPreconditions.CheckNotNull(value, "value");
      }
    }

    /// <summary>Field number for the "pricer" field.</summary>
    public const int PricerFieldNumber = 2;
    private string pricer_ = "";
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public string Pricer {
      get { return pricer_; }
      set {
        pricer_ = pb::ProtoPreconditions.CheckNotNull(value, "value");
      }
    }

    /// <summary>Field number for the "underlying" field.</summary>
    public const int UnderlyingFieldNumber = 3;
    private string underlying_ = "";
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public string Underlying {
      get { return underlying_; }
      set {
        underlying_ = pb::ProtoPreconditions.CheckNotNull(value, "value");
      }
    }

    /// <summary>Field number for the "delta_limit" field.</summary>
    public const int DeltaLimitFieldNumber = 4;
    private double deltaLimit_;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public double DeltaLimit {
      get { return deltaLimit_; }
      set {
        deltaLimit_ = value;
      }
    }

    /// <summary>Field number for the "order_limit" field.</summary>
    public const int OrderLimitFieldNumber = 5;
    private int orderLimit_;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public int OrderLimit {
      get { return orderLimit_; }
      set {
        orderLimit_ = value;
      }
    }

    /// <summary>Field number for the "trade_limit" field.</summary>
    public const int TradeLimitFieldNumber = 6;
    private int tradeLimit_;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public int TradeLimit {
      get { return tradeLimit_; }
      set {
        tradeLimit_ = value;
      }
    }

    /// <summary>Field number for the "bid_volume" field.</summary>
    public const int BidVolumeFieldNumber = 7;
    private int bidVolume_;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public int BidVolume {
      get { return bidVolume_; }
      set {
        bidVolume_ = value;
      }
    }

    /// <summary>Field number for the "ask_volume" field.</summary>
    public const int AskVolumeFieldNumber = 8;
    private int askVolume_;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public int AskVolume {
      get { return askVolume_; }
      set {
        askVolume_ = value;
      }
    }

    /// <summary>Field number for the "response_volume" field.</summary>
    public const int ResponseVolumeFieldNumber = 9;
    private int responseVolume_;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public int ResponseVolume {
      get { return responseVolume_; }
      set {
        responseVolume_ = value;
      }
    }

    /// <summary>Field number for the "depth" field.</summary>
    public const int DepthFieldNumber = 10;
    private int depth_;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public int Depth {
      get { return depth_; }
      set {
        depth_ = value;
      }
    }

    /// <summary>Field number for the "refill_times" field.</summary>
    public const int RefillTimesFieldNumber = 11;
    private int refillTimes_;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public int RefillTimes {
      get { return refillTimes_; }
      set {
        refillTimes_ = value;
      }
    }

    /// <summary>Field number for the "wide_spread" field.</summary>
    public const int WideSpreadFieldNumber = 12;
    private bool wideSpread_;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public bool WideSpread {
      get { return wideSpread_; }
      set {
        wideSpread_ = value;
      }
    }

    /// <summary>Field number for the "protection" field.</summary>
    public const int ProtectionFieldNumber = 13;
    private bool protection_;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public bool Protection {
      get { return protection_; }
      set {
        protection_ = value;
      }
    }

    /// <summary>Field number for the "options" field.</summary>
    public const int OptionsFieldNumber = 14;
    private static readonly pb::FieldCodec<string> _repeated_options_codec
        = pb::FieldCodec.ForString(114);
    private readonly pbc::RepeatedField<string> options_ = new pbc::RepeatedField<string>();
    /// <summary>
    ///repeated QuoterRecord records = 14;
    /// </summary>
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public pbc::RepeatedField<string> Options {
      get { return options_; }
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public override bool Equals(object other) {
      return Equals(other as QuoterSpec);
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public bool Equals(QuoterSpec other) {
      if (ReferenceEquals(other, null)) {
        return false;
      }
      if (ReferenceEquals(other, this)) {
        return true;
      }
      if (Name != other.Name) return false;
      if (Pricer != other.Pricer) return false;
      if (Underlying != other.Underlying) return false;
      if (!pbc::ProtobufEqualityComparers.BitwiseDoubleEqualityComparer.Equals(DeltaLimit, other.DeltaLimit)) return false;
      if (OrderLimit != other.OrderLimit) return false;
      if (TradeLimit != other.TradeLimit) return false;
      if (BidVolume != other.BidVolume) return false;
      if (AskVolume != other.AskVolume) return false;
      if (ResponseVolume != other.ResponseVolume) return false;
      if (Depth != other.Depth) return false;
      if (RefillTimes != other.RefillTimes) return false;
      if (WideSpread != other.WideSpread) return false;
      if (Protection != other.Protection) return false;
      if(!options_.Equals(other.options_)) return false;
      return Equals(_unknownFields, other._unknownFields);
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public override int GetHashCode() {
      int hash = 1;
      if (Name.Length != 0) hash ^= Name.GetHashCode();
      if (Pricer.Length != 0) hash ^= Pricer.GetHashCode();
      if (Underlying.Length != 0) hash ^= Underlying.GetHashCode();
      if (DeltaLimit != 0D) hash ^= pbc::ProtobufEqualityComparers.BitwiseDoubleEqualityComparer.GetHashCode(DeltaLimit);
      if (OrderLimit != 0) hash ^= OrderLimit.GetHashCode();
      if (TradeLimit != 0) hash ^= TradeLimit.GetHashCode();
      if (BidVolume != 0) hash ^= BidVolume.GetHashCode();
      if (AskVolume != 0) hash ^= AskVolume.GetHashCode();
      if (ResponseVolume != 0) hash ^= ResponseVolume.GetHashCode();
      if (Depth != 0) hash ^= Depth.GetHashCode();
      if (RefillTimes != 0) hash ^= RefillTimes.GetHashCode();
      if (WideSpread != false) hash ^= WideSpread.GetHashCode();
      if (Protection != false) hash ^= Protection.GetHashCode();
      hash ^= options_.GetHashCode();
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
      if (Name.Length != 0) {
        output.WriteRawTag(10);
        output.WriteString(Name);
      }
      if (Pricer.Length != 0) {
        output.WriteRawTag(18);
        output.WriteString(Pricer);
      }
      if (Underlying.Length != 0) {
        output.WriteRawTag(26);
        output.WriteString(Underlying);
      }
      if (DeltaLimit != 0D) {
        output.WriteRawTag(33);
        output.WriteDouble(DeltaLimit);
      }
      if (OrderLimit != 0) {
        output.WriteRawTag(40);
        output.WriteInt32(OrderLimit);
      }
      if (TradeLimit != 0) {
        output.WriteRawTag(48);
        output.WriteInt32(TradeLimit);
      }
      if (BidVolume != 0) {
        output.WriteRawTag(56);
        output.WriteInt32(BidVolume);
      }
      if (AskVolume != 0) {
        output.WriteRawTag(64);
        output.WriteInt32(AskVolume);
      }
      if (ResponseVolume != 0) {
        output.WriteRawTag(72);
        output.WriteInt32(ResponseVolume);
      }
      if (Depth != 0) {
        output.WriteRawTag(80);
        output.WriteInt32(Depth);
      }
      if (RefillTimes != 0) {
        output.WriteRawTag(88);
        output.WriteInt32(RefillTimes);
      }
      if (WideSpread != false) {
        output.WriteRawTag(96);
        output.WriteBool(WideSpread);
      }
      if (Protection != false) {
        output.WriteRawTag(104);
        output.WriteBool(Protection);
      }
      options_.WriteTo(output, _repeated_options_codec);
      if (_unknownFields != null) {
        _unknownFields.WriteTo(output);
      }
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public int CalculateSize() {
      int size = 0;
      if (Name.Length != 0) {
        size += 1 + pb::CodedOutputStream.ComputeStringSize(Name);
      }
      if (Pricer.Length != 0) {
        size += 1 + pb::CodedOutputStream.ComputeStringSize(Pricer);
      }
      if (Underlying.Length != 0) {
        size += 1 + pb::CodedOutputStream.ComputeStringSize(Underlying);
      }
      if (DeltaLimit != 0D) {
        size += 1 + 8;
      }
      if (OrderLimit != 0) {
        size += 1 + pb::CodedOutputStream.ComputeInt32Size(OrderLimit);
      }
      if (TradeLimit != 0) {
        size += 1 + pb::CodedOutputStream.ComputeInt32Size(TradeLimit);
      }
      if (BidVolume != 0) {
        size += 1 + pb::CodedOutputStream.ComputeInt32Size(BidVolume);
      }
      if (AskVolume != 0) {
        size += 1 + pb::CodedOutputStream.ComputeInt32Size(AskVolume);
      }
      if (ResponseVolume != 0) {
        size += 1 + pb::CodedOutputStream.ComputeInt32Size(ResponseVolume);
      }
      if (Depth != 0) {
        size += 1 + pb::CodedOutputStream.ComputeInt32Size(Depth);
      }
      if (RefillTimes != 0) {
        size += 1 + pb::CodedOutputStream.ComputeInt32Size(RefillTimes);
      }
      if (WideSpread != false) {
        size += 1 + 1;
      }
      if (Protection != false) {
        size += 1 + 1;
      }
      size += options_.CalculateSize(_repeated_options_codec);
      if (_unknownFields != null) {
        size += _unknownFields.CalculateSize();
      }
      return size;
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public void MergeFrom(QuoterSpec other) {
      if (other == null) {
        return;
      }
      if (other.Name.Length != 0) {
        Name = other.Name;
      }
      if (other.Pricer.Length != 0) {
        Pricer = other.Pricer;
      }
      if (other.Underlying.Length != 0) {
        Underlying = other.Underlying;
      }
      if (other.DeltaLimit != 0D) {
        DeltaLimit = other.DeltaLimit;
      }
      if (other.OrderLimit != 0) {
        OrderLimit = other.OrderLimit;
      }
      if (other.TradeLimit != 0) {
        TradeLimit = other.TradeLimit;
      }
      if (other.BidVolume != 0) {
        BidVolume = other.BidVolume;
      }
      if (other.AskVolume != 0) {
        AskVolume = other.AskVolume;
      }
      if (other.ResponseVolume != 0) {
        ResponseVolume = other.ResponseVolume;
      }
      if (other.Depth != 0) {
        Depth = other.Depth;
      }
      if (other.RefillTimes != 0) {
        RefillTimes = other.RefillTimes;
      }
      if (other.WideSpread != false) {
        WideSpread = other.WideSpread;
      }
      if (other.Protection != false) {
        Protection = other.Protection;
      }
      options_.Add(other.options_);
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
            Name = input.ReadString();
            break;
          }
          case 18: {
            Pricer = input.ReadString();
            break;
          }
          case 26: {
            Underlying = input.ReadString();
            break;
          }
          case 33: {
            DeltaLimit = input.ReadDouble();
            break;
          }
          case 40: {
            OrderLimit = input.ReadInt32();
            break;
          }
          case 48: {
            TradeLimit = input.ReadInt32();
            break;
          }
          case 56: {
            BidVolume = input.ReadInt32();
            break;
          }
          case 64: {
            AskVolume = input.ReadInt32();
            break;
          }
          case 72: {
            ResponseVolume = input.ReadInt32();
            break;
          }
          case 80: {
            Depth = input.ReadInt32();
            break;
          }
          case 88: {
            RefillTimes = input.ReadInt32();
            break;
          }
          case 96: {
            WideSpread = input.ReadBool();
            break;
          }
          case 104: {
            Protection = input.ReadBool();
            break;
          }
          case 114: {
            options_.AddEntriesFrom(input, _repeated_options_codec);
            break;
          }
        }
      }
    }

  }

  public sealed partial class QuoterReq : pb::IMessage<QuoterReq> {
    private static readonly pb::MessageParser<QuoterReq> _parser = new pb::MessageParser<QuoterReq>(() => new QuoterReq());
    private pb::UnknownFieldSet _unknownFields;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public static pb::MessageParser<QuoterReq> Parser { get { return _parser; } }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public static pbr::MessageDescriptor Descriptor {
      get { return global::Proto.QuoterReflection.Descriptor.MessageTypes[1]; }
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    pbr::MessageDescriptor pb::IMessage.Descriptor {
      get { return Descriptor; }
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public QuoterReq() {
      OnConstruction();
    }

    partial void OnConstruction();

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public QuoterReq(QuoterReq other) : this() {
      type_ = other.type_;
      exchange_ = other.exchange_;
      user_ = other.user_;
      quoters_ = other.quoters_.Clone();
      _unknownFields = pb::UnknownFieldSet.Clone(other._unknownFields);
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public QuoterReq Clone() {
      return new QuoterReq(this);
    }

    /// <summary>Field number for the "type" field.</summary>
    public const int TypeFieldNumber = 1;
    private global::Proto.RequestType type_ = 0;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public global::Proto.RequestType Type {
      get { return type_; }
      set {
        type_ = value;
      }
    }

    /// <summary>Field number for the "exchange" field.</summary>
    public const int ExchangeFieldNumber = 2;
    private global::Proto.Exchange exchange_ = 0;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public global::Proto.Exchange Exchange {
      get { return exchange_; }
      set {
        exchange_ = value;
      }
    }

    /// <summary>Field number for the "user" field.</summary>
    public const int UserFieldNumber = 3;
    private string user_ = "";
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public string User {
      get { return user_; }
      set {
        user_ = pb::ProtoPreconditions.CheckNotNull(value, "value");
      }
    }

    /// <summary>Field number for the "quoters" field.</summary>
    public const int QuotersFieldNumber = 4;
    private static readonly pb::FieldCodec<global::Proto.QuoterSpec> _repeated_quoters_codec
        = pb::FieldCodec.ForMessage(34, global::Proto.QuoterSpec.Parser);
    private readonly pbc::RepeatedField<global::Proto.QuoterSpec> quoters_ = new pbc::RepeatedField<global::Proto.QuoterSpec>();
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public pbc::RepeatedField<global::Proto.QuoterSpec> Quoters {
      get { return quoters_; }
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public override bool Equals(object other) {
      return Equals(other as QuoterReq);
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public bool Equals(QuoterReq other) {
      if (ReferenceEquals(other, null)) {
        return false;
      }
      if (ReferenceEquals(other, this)) {
        return true;
      }
      if (Type != other.Type) return false;
      if (Exchange != other.Exchange) return false;
      if (User != other.User) return false;
      if(!quoters_.Equals(other.quoters_)) return false;
      return Equals(_unknownFields, other._unknownFields);
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public override int GetHashCode() {
      int hash = 1;
      if (Type != 0) hash ^= Type.GetHashCode();
      if (Exchange != 0) hash ^= Exchange.GetHashCode();
      if (User.Length != 0) hash ^= User.GetHashCode();
      hash ^= quoters_.GetHashCode();
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
      if (Exchange != 0) {
        output.WriteRawTag(16);
        output.WriteEnum((int) Exchange);
      }
      if (User.Length != 0) {
        output.WriteRawTag(26);
        output.WriteString(User);
      }
      quoters_.WriteTo(output, _repeated_quoters_codec);
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
      if (Exchange != 0) {
        size += 1 + pb::CodedOutputStream.ComputeEnumSize((int) Exchange);
      }
      if (User.Length != 0) {
        size += 1 + pb::CodedOutputStream.ComputeStringSize(User);
      }
      size += quoters_.CalculateSize(_repeated_quoters_codec);
      if (_unknownFields != null) {
        size += _unknownFields.CalculateSize();
      }
      return size;
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public void MergeFrom(QuoterReq other) {
      if (other == null) {
        return;
      }
      if (other.Type != 0) {
        Type = other.Type;
      }
      if (other.Exchange != 0) {
        Exchange = other.Exchange;
      }
      if (other.User.Length != 0) {
        User = other.User;
      }
      quoters_.Add(other.quoters_);
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
            type_ = (global::Proto.RequestType) input.ReadEnum();
            break;
          }
          case 16: {
            exchange_ = (global::Proto.Exchange) input.ReadEnum();
            break;
          }
          case 26: {
            User = input.ReadString();
            break;
          }
          case 34: {
            quoters_.AddEntriesFrom(input, _repeated_quoters_codec);
            break;
          }
        }
      }
    }

  }

  public sealed partial class QuoterRep : pb::IMessage<QuoterRep> {
    private static readonly pb::MessageParser<QuoterRep> _parser = new pb::MessageParser<QuoterRep>(() => new QuoterRep());
    private pb::UnknownFieldSet _unknownFields;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public static pb::MessageParser<QuoterRep> Parser { get { return _parser; } }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public static pbr::MessageDescriptor Descriptor {
      get { return global::Proto.QuoterReflection.Descriptor.MessageTypes[2]; }
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    pbr::MessageDescriptor pb::IMessage.Descriptor {
      get { return Descriptor; }
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public QuoterRep() {
      OnConstruction();
    }

    partial void OnConstruction();

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public QuoterRep(QuoterRep other) : this() {
      quoters_ = other.quoters_.Clone();
      Result = other.result_ != null ? other.Result.Clone() : null;
      _unknownFields = pb::UnknownFieldSet.Clone(other._unknownFields);
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public QuoterRep Clone() {
      return new QuoterRep(this);
    }

    /// <summary>Field number for the "quoters" field.</summary>
    public const int QuotersFieldNumber = 1;
    private static readonly pb::FieldCodec<global::Proto.QuoterSpec> _repeated_quoters_codec
        = pb::FieldCodec.ForMessage(10, global::Proto.QuoterSpec.Parser);
    private readonly pbc::RepeatedField<global::Proto.QuoterSpec> quoters_ = new pbc::RepeatedField<global::Proto.QuoterSpec>();
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public pbc::RepeatedField<global::Proto.QuoterSpec> Quoters {
      get { return quoters_; }
    }

    /// <summary>Field number for the "result" field.</summary>
    public const int ResultFieldNumber = 2;
    private global::Proto.Reply result_;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public global::Proto.Reply Result {
      get { return result_; }
      set {
        result_ = value;
      }
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public override bool Equals(object other) {
      return Equals(other as QuoterRep);
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public bool Equals(QuoterRep other) {
      if (ReferenceEquals(other, null)) {
        return false;
      }
      if (ReferenceEquals(other, this)) {
        return true;
      }
      if(!quoters_.Equals(other.quoters_)) return false;
      if (!object.Equals(Result, other.Result)) return false;
      return Equals(_unknownFields, other._unknownFields);
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public override int GetHashCode() {
      int hash = 1;
      hash ^= quoters_.GetHashCode();
      if (result_ != null) hash ^= Result.GetHashCode();
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
      quoters_.WriteTo(output, _repeated_quoters_codec);
      if (result_ != null) {
        output.WriteRawTag(18);
        output.WriteMessage(Result);
      }
      if (_unknownFields != null) {
        _unknownFields.WriteTo(output);
      }
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public int CalculateSize() {
      int size = 0;
      size += quoters_.CalculateSize(_repeated_quoters_codec);
      if (result_ != null) {
        size += 1 + pb::CodedOutputStream.ComputeMessageSize(Result);
      }
      if (_unknownFields != null) {
        size += _unknownFields.CalculateSize();
      }
      return size;
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public void MergeFrom(QuoterRep other) {
      if (other == null) {
        return;
      }
      quoters_.Add(other.quoters_);
      if (other.result_ != null) {
        if (result_ == null) {
          result_ = new global::Proto.Reply();
        }
        Result.MergeFrom(other.Result);
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
            quoters_.AddEntriesFrom(input, _repeated_quoters_codec);
            break;
          }
          case 18: {
            if (result_ == null) {
              result_ = new global::Proto.Reply();
            }
            input.ReadMessage(result_);
            break;
          }
        }
      }
    }

  }

  #endregion

}

#endregion Designer generated code