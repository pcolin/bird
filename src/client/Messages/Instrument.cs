// <auto-generated>
//     Generated by the protocol buffer compiler.  DO NOT EDIT!
//     source: Instrument.proto
// </auto-generated>
#pragma warning disable 1591, 0612, 3021
#region Designer generated code

using pb = global::Google.Protobuf;
using pbc = global::Google.Protobuf.Collections;
using pbr = global::Google.Protobuf.Reflection;
using scg = global::System.Collections.Generic;
namespace Proto {

  /// <summary>Holder for reflection information generated from Instrument.proto</summary>
  public static partial class InstrumentReflection {

    #region Descriptor
    /// <summary>File descriptor for Instrument.proto</summary>
    public static pbr::FileDescriptor Descriptor {
      get { return descriptor; }
    }
    private static pbr::FileDescriptor descriptor;

    static InstrumentReflection() {
      byte[] descriptorData = global::System.Convert.FromBase64String(
          string.Concat(
            "ChBJbnN0cnVtZW50LnByb3RvEgVQcm90bxoORXhjaGFuZ2UucHJvdG8aDVJl",
            "cXVlc3QucHJvdG8aC1JlcGx5LnByb3RvGg5DdXJyZW5jeS5wcm90byKdAwoK",
            "SW5zdHJ1bWVudBIKCgJpZBgBIAEoCRIOCgZzeW1ib2wYAiABKAkSIQoIZXhj",
            "aGFuZ2UYAyABKA4yDy5Qcm90by5FeGNoYW5nZRIjCgR0eXBlGAQgASgOMhUu",
            "UHJvdG8uSW5zdHJ1bWVudFR5cGUSIQoIY3VycmVuY3kYBSABKA4yDy5Qcm90",
            "by5DdXJyZW5jeRISCgp1bmRlcmx5aW5nGAYgASgJEhgKEGhlZGdlX3VuZGVy",
            "bHlpbmcYByABKAkSDAoEdGljaxgIIAEoARISCgptdWx0aXBsaWVyGAkgASgB",
            "Eg8KB2hpZ2hlc3QYCiABKAESDgoGbG93ZXN0GAsgASgBEiMKCGNhbGxfcHV0",
            "GAwgASgOMhEuUHJvdG8uT3B0aW9uVHlwZRIQCghtYXR1cml0eRgNIAEoCRIl",
            "CghleGVyY2lzZRgOIAEoDjITLlByb3RvLkV4ZXJjaXNlVHlwZRIpCgpzZXR0",
            "bGVtZW50GA8gASgOMhUuUHJvdG8uU2V0dGxlbWVudFR5cGUSDgoGc3RyaWtl",
            "GBAgASgBIooBCg1JbnN0cnVtZW50UmVxEiAKBHR5cGUYASABKA4yEi5Qcm90",
            "by5SZXF1ZXN0VHlwZRImCgtpbnN0cnVtZW50cxgCIAMoCzIRLlByb3RvLklu",
            "c3RydW1lbnQSIQoIZXhjaGFuZ2UYAyABKA4yDy5Qcm90by5FeGNoYW5nZRIM",
            "CgR1c2VyGAQgASgJIlUKDUluc3RydW1lbnRSZXASJgoLaW5zdHJ1bWVudHMY",
            "ASADKAsyES5Qcm90by5JbnN0cnVtZW50EhwKBnJlc3VsdBgCIAEoCzIMLlBy",
            "b3RvLlJlcGx5KjMKDkluc3RydW1lbnRUeXBlEgkKBVN0b2NrEAASCgoGRnV0",
            "dXJlEAESCgoGT3B0aW9uEAIqjQEKEEluc3RydW1lbnRTdGF0dXMSCgoGVW5r",
            "b3duEAASCgoGQ2xvc2VkEAESCwoHUHJlT3BlbhACEhIKDk9wZW5pbmdBdWN0",
            "aW9uEAMSCwoHVHJhZGluZxAEEggKBEZ1c2UQBRILCgdBdWN0aW9uEAYSEgoO",
            "Q2xvc2luZ0F1Y3Rpb24QBxIICgRIYWx0EAgqHwoKT3B0aW9uVHlwZRIICgRD",
            "YWxsEAASBwoDUHV0EAEqKgoMRXhlcmNpc2VUeXBlEgwKCEV1cm9wZWFuEAAS",
            "DAoIQW1lcmljYW4QASo8Cg5TZXR0bGVtZW50VHlwZRISCg5DYXNoU2V0dGxl",
            "bWVudBAAEhYKElBoeXNpY2FsU2V0dGxlbWVudBABYgZwcm90bzM="));
      descriptor = pbr::FileDescriptor.FromGeneratedCode(descriptorData,
          new pbr::FileDescriptor[] { global::Proto.ExchangeReflection.Descriptor, global::Proto.RequestReflection.Descriptor, global::Proto.ReplyReflection.Descriptor, global::Proto.CurrencyReflection.Descriptor, },
          new pbr::GeneratedClrTypeInfo(new[] {typeof(global::Proto.InstrumentType), typeof(global::Proto.InstrumentStatus), typeof(global::Proto.OptionType), typeof(global::Proto.ExerciseType), typeof(global::Proto.SettlementType), }, new pbr::GeneratedClrTypeInfo[] {
            new pbr::GeneratedClrTypeInfo(typeof(global::Proto.Instrument), global::Proto.Instrument.Parser, new[]{ "Id", "Symbol", "Exchange", "Type", "Currency", "Underlying", "HedgeUnderlying", "Tick", "Multiplier", "Highest", "Lowest", "CallPut", "Maturity", "Exercise", "Settlement", "Strike" }, null, null, null),
            new pbr::GeneratedClrTypeInfo(typeof(global::Proto.InstrumentReq), global::Proto.InstrumentReq.Parser, new[]{ "Type", "Instruments", "Exchange", "User" }, null, null, null),
            new pbr::GeneratedClrTypeInfo(typeof(global::Proto.InstrumentRep), global::Proto.InstrumentRep.Parser, new[]{ "Instruments", "Result" }, null, null, null)
          }));
    }
    #endregion

  }
  #region Enums
  public enum InstrumentType {
    [pbr::OriginalName("Stock")] Stock = 0,
    [pbr::OriginalName("Future")] Future = 1,
    [pbr::OriginalName("Option")] Option = 2,
  }

  public enum InstrumentStatus {
    [pbr::OriginalName("Unkown")] Unkown = 0,
    [pbr::OriginalName("Closed")] Closed = 1,
    [pbr::OriginalName("PreOpen")] PreOpen = 2,
    [pbr::OriginalName("OpeningAuction")] OpeningAuction = 3,
    [pbr::OriginalName("Trading")] Trading = 4,
    [pbr::OriginalName("Fuse")] Fuse = 5,
    [pbr::OriginalName("Auction")] Auction = 6,
    [pbr::OriginalName("ClosingAuction")] ClosingAuction = 7,
    [pbr::OriginalName("Halt")] Halt = 8,
  }

  public enum OptionType {
    [pbr::OriginalName("Call")] Call = 0,
    [pbr::OriginalName("Put")] Put = 1,
  }

  public enum ExerciseType {
    [pbr::OriginalName("European")] European = 0,
    [pbr::OriginalName("American")] American = 1,
  }

  public enum SettlementType {
    [pbr::OriginalName("CashSettlement")] CashSettlement = 0,
    [pbr::OriginalName("PhysicalSettlement")] PhysicalSettlement = 1,
  }

  #endregion

  #region Messages
  public sealed partial class Instrument : pb::IMessage<Instrument> {
    private static readonly pb::MessageParser<Instrument> _parser = new pb::MessageParser<Instrument>(() => new Instrument());
    private pb::UnknownFieldSet _unknownFields;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public static pb::MessageParser<Instrument> Parser { get { return _parser; } }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public static pbr::MessageDescriptor Descriptor {
      get { return global::Proto.InstrumentReflection.Descriptor.MessageTypes[0]; }
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    pbr::MessageDescriptor pb::IMessage.Descriptor {
      get { return Descriptor; }
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public Instrument() {
      OnConstruction();
    }

    partial void OnConstruction();

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public Instrument(Instrument other) : this() {
      id_ = other.id_;
      symbol_ = other.symbol_;
      exchange_ = other.exchange_;
      type_ = other.type_;
      currency_ = other.currency_;
      underlying_ = other.underlying_;
      hedgeUnderlying_ = other.hedgeUnderlying_;
      tick_ = other.tick_;
      multiplier_ = other.multiplier_;
      highest_ = other.highest_;
      lowest_ = other.lowest_;
      callPut_ = other.callPut_;
      maturity_ = other.maturity_;
      exercise_ = other.exercise_;
      settlement_ = other.settlement_;
      strike_ = other.strike_;
      _unknownFields = pb::UnknownFieldSet.Clone(other._unknownFields);
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public Instrument Clone() {
      return new Instrument(this);
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

    /// <summary>Field number for the "symbol" field.</summary>
    public const int SymbolFieldNumber = 2;
    private string symbol_ = "";
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public string Symbol {
      get { return symbol_; }
      set {
        symbol_ = pb::ProtoPreconditions.CheckNotNull(value, "value");
      }
    }

    /// <summary>Field number for the "exchange" field.</summary>
    public const int ExchangeFieldNumber = 3;
    private global::Proto.Exchange exchange_ = 0;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public global::Proto.Exchange Exchange {
      get { return exchange_; }
      set {
        exchange_ = value;
      }
    }

    /// <summary>Field number for the "type" field.</summary>
    public const int TypeFieldNumber = 4;
    private global::Proto.InstrumentType type_ = 0;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public global::Proto.InstrumentType Type {
      get { return type_; }
      set {
        type_ = value;
      }
    }

    /// <summary>Field number for the "currency" field.</summary>
    public const int CurrencyFieldNumber = 5;
    private global::Proto.Currency currency_ = 0;
    /// <summary>
    ///InstrumentStatus status = 5;
    /// </summary>
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public global::Proto.Currency Currency {
      get { return currency_; }
      set {
        currency_ = value;
      }
    }

    /// <summary>Field number for the "underlying" field.</summary>
    public const int UnderlyingFieldNumber = 6;
    private string underlying_ = "";
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public string Underlying {
      get { return underlying_; }
      set {
        underlying_ = pb::ProtoPreconditions.CheckNotNull(value, "value");
      }
    }

    /// <summary>Field number for the "hedge_underlying" field.</summary>
    public const int HedgeUnderlyingFieldNumber = 7;
    private string hedgeUnderlying_ = "";
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public string HedgeUnderlying {
      get { return hedgeUnderlying_; }
      set {
        hedgeUnderlying_ = pb::ProtoPreconditions.CheckNotNull(value, "value");
      }
    }

    /// <summary>Field number for the "tick" field.</summary>
    public const int TickFieldNumber = 8;
    private double tick_;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public double Tick {
      get { return tick_; }
      set {
        tick_ = value;
      }
    }

    /// <summary>Field number for the "multiplier" field.</summary>
    public const int MultiplierFieldNumber = 9;
    private double multiplier_;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public double Multiplier {
      get { return multiplier_; }
      set {
        multiplier_ = value;
      }
    }

    /// <summary>Field number for the "highest" field.</summary>
    public const int HighestFieldNumber = 10;
    private double highest_;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public double Highest {
      get { return highest_; }
      set {
        highest_ = value;
      }
    }

    /// <summary>Field number for the "lowest" field.</summary>
    public const int LowestFieldNumber = 11;
    private double lowest_;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public double Lowest {
      get { return lowest_; }
      set {
        lowest_ = value;
      }
    }

    /// <summary>Field number for the "call_put" field.</summary>
    public const int CallPutFieldNumber = 12;
    private global::Proto.OptionType callPut_ = 0;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public global::Proto.OptionType CallPut {
      get { return callPut_; }
      set {
        callPut_ = value;
      }
    }

    /// <summary>Field number for the "maturity" field.</summary>
    public const int MaturityFieldNumber = 13;
    private string maturity_ = "";
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public string Maturity {
      get { return maturity_; }
      set {
        maturity_ = pb::ProtoPreconditions.CheckNotNull(value, "value");
      }
    }

    /// <summary>Field number for the "exercise" field.</summary>
    public const int ExerciseFieldNumber = 14;
    private global::Proto.ExerciseType exercise_ = 0;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public global::Proto.ExerciseType Exercise {
      get { return exercise_; }
      set {
        exercise_ = value;
      }
    }

    /// <summary>Field number for the "settlement" field.</summary>
    public const int SettlementFieldNumber = 15;
    private global::Proto.SettlementType settlement_ = 0;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public global::Proto.SettlementType Settlement {
      get { return settlement_; }
      set {
        settlement_ = value;
      }
    }

    /// <summary>Field number for the "strike" field.</summary>
    public const int StrikeFieldNumber = 16;
    private double strike_;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public double Strike {
      get { return strike_; }
      set {
        strike_ = value;
      }
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public override bool Equals(object other) {
      return Equals(other as Instrument);
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public bool Equals(Instrument other) {
      if (ReferenceEquals(other, null)) {
        return false;
      }
      if (ReferenceEquals(other, this)) {
        return true;
      }
      if (Id != other.Id) return false;
      if (Symbol != other.Symbol) return false;
      if (Exchange != other.Exchange) return false;
      if (Type != other.Type) return false;
      if (Currency != other.Currency) return false;
      if (Underlying != other.Underlying) return false;
      if (HedgeUnderlying != other.HedgeUnderlying) return false;
      if (!pbc::ProtobufEqualityComparers.BitwiseDoubleEqualityComparer.Equals(Tick, other.Tick)) return false;
      if (!pbc::ProtobufEqualityComparers.BitwiseDoubleEqualityComparer.Equals(Multiplier, other.Multiplier)) return false;
      if (!pbc::ProtobufEqualityComparers.BitwiseDoubleEqualityComparer.Equals(Highest, other.Highest)) return false;
      if (!pbc::ProtobufEqualityComparers.BitwiseDoubleEqualityComparer.Equals(Lowest, other.Lowest)) return false;
      if (CallPut != other.CallPut) return false;
      if (Maturity != other.Maturity) return false;
      if (Exercise != other.Exercise) return false;
      if (Settlement != other.Settlement) return false;
      if (!pbc::ProtobufEqualityComparers.BitwiseDoubleEqualityComparer.Equals(Strike, other.Strike)) return false;
      return Equals(_unknownFields, other._unknownFields);
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public override int GetHashCode() {
      int hash = 1;
      if (Id.Length != 0) hash ^= Id.GetHashCode();
      if (Symbol.Length != 0) hash ^= Symbol.GetHashCode();
      if (Exchange != 0) hash ^= Exchange.GetHashCode();
      if (Type != 0) hash ^= Type.GetHashCode();
      if (Currency != 0) hash ^= Currency.GetHashCode();
      if (Underlying.Length != 0) hash ^= Underlying.GetHashCode();
      if (HedgeUnderlying.Length != 0) hash ^= HedgeUnderlying.GetHashCode();
      if (Tick != 0D) hash ^= pbc::ProtobufEqualityComparers.BitwiseDoubleEqualityComparer.GetHashCode(Tick);
      if (Multiplier != 0D) hash ^= pbc::ProtobufEqualityComparers.BitwiseDoubleEqualityComparer.GetHashCode(Multiplier);
      if (Highest != 0D) hash ^= pbc::ProtobufEqualityComparers.BitwiseDoubleEqualityComparer.GetHashCode(Highest);
      if (Lowest != 0D) hash ^= pbc::ProtobufEqualityComparers.BitwiseDoubleEqualityComparer.GetHashCode(Lowest);
      if (CallPut != 0) hash ^= CallPut.GetHashCode();
      if (Maturity.Length != 0) hash ^= Maturity.GetHashCode();
      if (Exercise != 0) hash ^= Exercise.GetHashCode();
      if (Settlement != 0) hash ^= Settlement.GetHashCode();
      if (Strike != 0D) hash ^= pbc::ProtobufEqualityComparers.BitwiseDoubleEqualityComparer.GetHashCode(Strike);
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
      if (Symbol.Length != 0) {
        output.WriteRawTag(18);
        output.WriteString(Symbol);
      }
      if (Exchange != 0) {
        output.WriteRawTag(24);
        output.WriteEnum((int) Exchange);
      }
      if (Type != 0) {
        output.WriteRawTag(32);
        output.WriteEnum((int) Type);
      }
      if (Currency != 0) {
        output.WriteRawTag(40);
        output.WriteEnum((int) Currency);
      }
      if (Underlying.Length != 0) {
        output.WriteRawTag(50);
        output.WriteString(Underlying);
      }
      if (HedgeUnderlying.Length != 0) {
        output.WriteRawTag(58);
        output.WriteString(HedgeUnderlying);
      }
      if (Tick != 0D) {
        output.WriteRawTag(65);
        output.WriteDouble(Tick);
      }
      if (Multiplier != 0D) {
        output.WriteRawTag(73);
        output.WriteDouble(Multiplier);
      }
      if (Highest != 0D) {
        output.WriteRawTag(81);
        output.WriteDouble(Highest);
      }
      if (Lowest != 0D) {
        output.WriteRawTag(89);
        output.WriteDouble(Lowest);
      }
      if (CallPut != 0) {
        output.WriteRawTag(96);
        output.WriteEnum((int) CallPut);
      }
      if (Maturity.Length != 0) {
        output.WriteRawTag(106);
        output.WriteString(Maturity);
      }
      if (Exercise != 0) {
        output.WriteRawTag(112);
        output.WriteEnum((int) Exercise);
      }
      if (Settlement != 0) {
        output.WriteRawTag(120);
        output.WriteEnum((int) Settlement);
      }
      if (Strike != 0D) {
        output.WriteRawTag(129, 1);
        output.WriteDouble(Strike);
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
      if (Symbol.Length != 0) {
        size += 1 + pb::CodedOutputStream.ComputeStringSize(Symbol);
      }
      if (Exchange != 0) {
        size += 1 + pb::CodedOutputStream.ComputeEnumSize((int) Exchange);
      }
      if (Type != 0) {
        size += 1 + pb::CodedOutputStream.ComputeEnumSize((int) Type);
      }
      if (Currency != 0) {
        size += 1 + pb::CodedOutputStream.ComputeEnumSize((int) Currency);
      }
      if (Underlying.Length != 0) {
        size += 1 + pb::CodedOutputStream.ComputeStringSize(Underlying);
      }
      if (HedgeUnderlying.Length != 0) {
        size += 1 + pb::CodedOutputStream.ComputeStringSize(HedgeUnderlying);
      }
      if (Tick != 0D) {
        size += 1 + 8;
      }
      if (Multiplier != 0D) {
        size += 1 + 8;
      }
      if (Highest != 0D) {
        size += 1 + 8;
      }
      if (Lowest != 0D) {
        size += 1 + 8;
      }
      if (CallPut != 0) {
        size += 1 + pb::CodedOutputStream.ComputeEnumSize((int) CallPut);
      }
      if (Maturity.Length != 0) {
        size += 1 + pb::CodedOutputStream.ComputeStringSize(Maturity);
      }
      if (Exercise != 0) {
        size += 1 + pb::CodedOutputStream.ComputeEnumSize((int) Exercise);
      }
      if (Settlement != 0) {
        size += 1 + pb::CodedOutputStream.ComputeEnumSize((int) Settlement);
      }
      if (Strike != 0D) {
        size += 2 + 8;
      }
      if (_unknownFields != null) {
        size += _unknownFields.CalculateSize();
      }
      return size;
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public void MergeFrom(Instrument other) {
      if (other == null) {
        return;
      }
      if (other.Id.Length != 0) {
        Id = other.Id;
      }
      if (other.Symbol.Length != 0) {
        Symbol = other.Symbol;
      }
      if (other.Exchange != 0) {
        Exchange = other.Exchange;
      }
      if (other.Type != 0) {
        Type = other.Type;
      }
      if (other.Currency != 0) {
        Currency = other.Currency;
      }
      if (other.Underlying.Length != 0) {
        Underlying = other.Underlying;
      }
      if (other.HedgeUnderlying.Length != 0) {
        HedgeUnderlying = other.HedgeUnderlying;
      }
      if (other.Tick != 0D) {
        Tick = other.Tick;
      }
      if (other.Multiplier != 0D) {
        Multiplier = other.Multiplier;
      }
      if (other.Highest != 0D) {
        Highest = other.Highest;
      }
      if (other.Lowest != 0D) {
        Lowest = other.Lowest;
      }
      if (other.CallPut != 0) {
        CallPut = other.CallPut;
      }
      if (other.Maturity.Length != 0) {
        Maturity = other.Maturity;
      }
      if (other.Exercise != 0) {
        Exercise = other.Exercise;
      }
      if (other.Settlement != 0) {
        Settlement = other.Settlement;
      }
      if (other.Strike != 0D) {
        Strike = other.Strike;
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
            Symbol = input.ReadString();
            break;
          }
          case 24: {
            exchange_ = (global::Proto.Exchange) input.ReadEnum();
            break;
          }
          case 32: {
            type_ = (global::Proto.InstrumentType) input.ReadEnum();
            break;
          }
          case 40: {
            currency_ = (global::Proto.Currency) input.ReadEnum();
            break;
          }
          case 50: {
            Underlying = input.ReadString();
            break;
          }
          case 58: {
            HedgeUnderlying = input.ReadString();
            break;
          }
          case 65: {
            Tick = input.ReadDouble();
            break;
          }
          case 73: {
            Multiplier = input.ReadDouble();
            break;
          }
          case 81: {
            Highest = input.ReadDouble();
            break;
          }
          case 89: {
            Lowest = input.ReadDouble();
            break;
          }
          case 96: {
            callPut_ = (global::Proto.OptionType) input.ReadEnum();
            break;
          }
          case 106: {
            Maturity = input.ReadString();
            break;
          }
          case 112: {
            exercise_ = (global::Proto.ExerciseType) input.ReadEnum();
            break;
          }
          case 120: {
            settlement_ = (global::Proto.SettlementType) input.ReadEnum();
            break;
          }
          case 129: {
            Strike = input.ReadDouble();
            break;
          }
        }
      }
    }

  }

  public sealed partial class InstrumentReq : pb::IMessage<InstrumentReq> {
    private static readonly pb::MessageParser<InstrumentReq> _parser = new pb::MessageParser<InstrumentReq>(() => new InstrumentReq());
    private pb::UnknownFieldSet _unknownFields;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public static pb::MessageParser<InstrumentReq> Parser { get { return _parser; } }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public static pbr::MessageDescriptor Descriptor {
      get { return global::Proto.InstrumentReflection.Descriptor.MessageTypes[1]; }
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    pbr::MessageDescriptor pb::IMessage.Descriptor {
      get { return Descriptor; }
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public InstrumentReq() {
      OnConstruction();
    }

    partial void OnConstruction();

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public InstrumentReq(InstrumentReq other) : this() {
      type_ = other.type_;
      instruments_ = other.instruments_.Clone();
      exchange_ = other.exchange_;
      user_ = other.user_;
      _unknownFields = pb::UnknownFieldSet.Clone(other._unknownFields);
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public InstrumentReq Clone() {
      return new InstrumentReq(this);
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

    /// <summary>Field number for the "instruments" field.</summary>
    public const int InstrumentsFieldNumber = 2;
    private static readonly pb::FieldCodec<global::Proto.Instrument> _repeated_instruments_codec
        = pb::FieldCodec.ForMessage(18, global::Proto.Instrument.Parser);
    private readonly pbc::RepeatedField<global::Proto.Instrument> instruments_ = new pbc::RepeatedField<global::Proto.Instrument>();
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public pbc::RepeatedField<global::Proto.Instrument> Instruments {
      get { return instruments_; }
    }

    /// <summary>Field number for the "exchange" field.</summary>
    public const int ExchangeFieldNumber = 3;
    private global::Proto.Exchange exchange_ = 0;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public global::Proto.Exchange Exchange {
      get { return exchange_; }
      set {
        exchange_ = value;
      }
    }

    /// <summary>Field number for the "user" field.</summary>
    public const int UserFieldNumber = 4;
    private string user_ = "";
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public string User {
      get { return user_; }
      set {
        user_ = pb::ProtoPreconditions.CheckNotNull(value, "value");
      }
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public override bool Equals(object other) {
      return Equals(other as InstrumentReq);
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public bool Equals(InstrumentReq other) {
      if (ReferenceEquals(other, null)) {
        return false;
      }
      if (ReferenceEquals(other, this)) {
        return true;
      }
      if (Type != other.Type) return false;
      if(!instruments_.Equals(other.instruments_)) return false;
      if (Exchange != other.Exchange) return false;
      if (User != other.User) return false;
      return Equals(_unknownFields, other._unknownFields);
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public override int GetHashCode() {
      int hash = 1;
      if (Type != 0) hash ^= Type.GetHashCode();
      hash ^= instruments_.GetHashCode();
      if (Exchange != 0) hash ^= Exchange.GetHashCode();
      if (User.Length != 0) hash ^= User.GetHashCode();
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
      instruments_.WriteTo(output, _repeated_instruments_codec);
      if (Exchange != 0) {
        output.WriteRawTag(24);
        output.WriteEnum((int) Exchange);
      }
      if (User.Length != 0) {
        output.WriteRawTag(34);
        output.WriteString(User);
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
      size += instruments_.CalculateSize(_repeated_instruments_codec);
      if (Exchange != 0) {
        size += 1 + pb::CodedOutputStream.ComputeEnumSize((int) Exchange);
      }
      if (User.Length != 0) {
        size += 1 + pb::CodedOutputStream.ComputeStringSize(User);
      }
      if (_unknownFields != null) {
        size += _unknownFields.CalculateSize();
      }
      return size;
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public void MergeFrom(InstrumentReq other) {
      if (other == null) {
        return;
      }
      if (other.Type != 0) {
        Type = other.Type;
      }
      instruments_.Add(other.instruments_);
      if (other.Exchange != 0) {
        Exchange = other.Exchange;
      }
      if (other.User.Length != 0) {
        User = other.User;
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
            type_ = (global::Proto.RequestType) input.ReadEnum();
            break;
          }
          case 18: {
            instruments_.AddEntriesFrom(input, _repeated_instruments_codec);
            break;
          }
          case 24: {
            exchange_ = (global::Proto.Exchange) input.ReadEnum();
            break;
          }
          case 34: {
            User = input.ReadString();
            break;
          }
        }
      }
    }

  }

  public sealed partial class InstrumentRep : pb::IMessage<InstrumentRep> {
    private static readonly pb::MessageParser<InstrumentRep> _parser = new pb::MessageParser<InstrumentRep>(() => new InstrumentRep());
    private pb::UnknownFieldSet _unknownFields;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public static pb::MessageParser<InstrumentRep> Parser { get { return _parser; } }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public static pbr::MessageDescriptor Descriptor {
      get { return global::Proto.InstrumentReflection.Descriptor.MessageTypes[2]; }
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    pbr::MessageDescriptor pb::IMessage.Descriptor {
      get { return Descriptor; }
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public InstrumentRep() {
      OnConstruction();
    }

    partial void OnConstruction();

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public InstrumentRep(InstrumentRep other) : this() {
      instruments_ = other.instruments_.Clone();
      Result = other.result_ != null ? other.Result.Clone() : null;
      _unknownFields = pb::UnknownFieldSet.Clone(other._unknownFields);
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public InstrumentRep Clone() {
      return new InstrumentRep(this);
    }

    /// <summary>Field number for the "instruments" field.</summary>
    public const int InstrumentsFieldNumber = 1;
    private static readonly pb::FieldCodec<global::Proto.Instrument> _repeated_instruments_codec
        = pb::FieldCodec.ForMessage(10, global::Proto.Instrument.Parser);
    private readonly pbc::RepeatedField<global::Proto.Instrument> instruments_ = new pbc::RepeatedField<global::Proto.Instrument>();
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public pbc::RepeatedField<global::Proto.Instrument> Instruments {
      get { return instruments_; }
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
      return Equals(other as InstrumentRep);
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public bool Equals(InstrumentRep other) {
      if (ReferenceEquals(other, null)) {
        return false;
      }
      if (ReferenceEquals(other, this)) {
        return true;
      }
      if(!instruments_.Equals(other.instruments_)) return false;
      if (!object.Equals(Result, other.Result)) return false;
      return Equals(_unknownFields, other._unknownFields);
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public override int GetHashCode() {
      int hash = 1;
      hash ^= instruments_.GetHashCode();
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
      instruments_.WriteTo(output, _repeated_instruments_codec);
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
      size += instruments_.CalculateSize(_repeated_instruments_codec);
      if (result_ != null) {
        size += 1 + pb::CodedOutputStream.ComputeMessageSize(Result);
      }
      if (_unknownFields != null) {
        size += _unknownFields.CalculateSize();
      }
      return size;
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public void MergeFrom(InstrumentRep other) {
      if (other == null) {
        return;
      }
      instruments_.Add(other.instruments_);
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
            instruments_.AddEntriesFrom(input, _repeated_instruments_codec);
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