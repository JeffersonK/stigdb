/* <stig/mynde/binary_protocol.cc>

   Implements <stig/mynde/binary_protocol.h>

   Implementation notes:
     We swap in place on input but not output. For output we have a const guarantee to maintain.


   Copyright 2010-2014 Stig LLC

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License. */

#include <stig/mynde/binary_protocol.h>
#include <stig/mynde/protocol.h>

#include <base/not_implemented.h>
#include <io/endian.h>

using namespace Io;
using namespace Strm::Bin;
using namespace Stig::Mynde;

TIn &Stig::Mynde::operator>>(TIn &in, TRequestHeader &that) {
  in.ReadShallow(that);

  that.KeyLength = SwapEnds(that.KeyLength);
  that.Reserved = SwapEnds(that.Reserved);
  that.TotalBodyLength = SwapEnds(that.TotalBodyLength);
  //NOTE: Opaque is explicitly skipped
  that.Cas = SwapEnds(that.Cas);

  return in;
}

TOut &Stig::Mynde::operator<<(TOut &out, const TRequestHeader &that) {
  //TODO: Should we SwapEnds on ourselves then pass through?
  // The only problem if anyone happens to read our values during that time they'll get the incorrect results

  //TODO: This is a hack. But it works!
  TRequestHeader tmp(that);
  tmp.KeyLength = SwapEnds(tmp.KeyLength);
  tmp.Reserved = SwapEnds(tmp.Reserved);
  tmp.TotalBodyLength = SwapEnds(tmp.TotalBodyLength);
  //NOTE: Opaque is explicitly skipped
  tmp.Cas = SwapEnds(tmp.Cas);
  out.WriteShallow(tmp);

  return out;
}

TIn &Stig::Mynde::operator>>(TIn &in, TResponseHeader &that) {
  in.ReadShallow(that);

  that.KeyLength = SwapEnds(that.KeyLength);
  that.Status = SwapEnds(that.Status);
  that.TotalBodyLength = SwapEnds(that.TotalBodyLength);
  //NOTE: Opaque is explicitly skipped
  that.Cas = SwapEnds(that.Cas);

  return in;
}

TOut &Stig::Mynde::operator<<(TOut &out, const TResponseHeader &that) {

  //TODO: This is a hack. But it works!
  TResponseHeader tmp(that);
  tmp.KeyLength = SwapEnds(tmp.KeyLength);
  tmp.Status = SwapEnds(tmp.Status);
  tmp.TotalBodyLength = SwapEnds(tmp.TotalBodyLength);
  //NOTE: Opaque is explicitly skipped
  tmp.Cas = SwapEnds(tmp.Cas);

  out.WriteShallow(tmp);
  out.Flush();

  return out;
}

TRequest::TRequest(TIn &in) : Flags({false,false}), Opaque(0), Cas(0) {
  TRequestHeader header;
  in >> header;
  if (header.Magic != BinaryMagicRequest) {
    throw std::invalid_argument("Invalid magic byte");
  }
  // Validate fields, one at a time filling in our data representation from the header.
  // NOTE: Everything in header is untrusted.
  switch (header.Opcode) {
    case 0x00: // Get
      Opcode = TOpcode::Get;
      break;
    case 0x01: // Set
      Opcode = TOpcode::Set;
      break;
    case 0x02: // Add
      Opcode = TOpcode::Add;
      break;
    case 0x03: // Replace
      Opcode = TOpcode::Replace;
      break;
    case 0x04: // Delete
      Opcode = TOpcode::Delete;
      break;
    case 0x05: // Increment
      Opcode = TOpcode::Increment;
      break;
    case 0x06: // Decrement
      Opcode = TOpcode::Decrement;
      break;
    case 0x07: // Quit
      Opcode = TOpcode::Quit;
      break;
    case 0x08: // Flush
      Opcode = TOpcode::Flush;
      break;
    case 0x09: // GetQ
      Opcode = TOpcode::Get;
      Flags.Quiet = true;
      break;
    case 0x0A: // NoOp
      Opcode = TOpcode::NoOp;
      break;
    case 0x0B: // Version
      Opcode = TOpcode::Version;
      break;
    case 0x0C: // GetK
      Opcode = TOpcode::Get;
      Flags.Key = true;
      break;
    case 0x0D: // GetKQ
      Opcode = TOpcode::Get;
      Flags.Key = true;
      Flags.Quiet = true;
      break;
    case 0x0E: // Append
      Opcode = TOpcode::Append;
      break;
    case 0x0F: // Prepend
      Opcode = TOpcode::Prepend;
      break;
    case 0x10: // State
      Opcode = TOpcode::State;
      break;
    case 0x11: // SetQ
      Opcode = TOpcode::Set;
      Flags.Quiet = true;
      break;
    case 0x12: // AddQ
      Opcode = TOpcode::Add;
      Flags.Quiet = true;
      break;
    case 0x13: // ReplaceQ
      Opcode = TOpcode::Replace;
      Flags.Quiet = true;
      break;
    case 0x14: // DeleteQ
      Opcode = TOpcode::Delete;
      Flags.Quiet = true;
      break;
    case 0x15: // IncrementQ
      Opcode = TOpcode::Increment;
      Flags.Quiet = true;
      break;
    case 0x16: // DecrementQ
      Opcode = TOpcode::Decrement;
      Flags.Quiet = true;
      break;
    case 0x17: // QuitQ
      Opcode = TOpcode::Quit;
      Flags.Quiet = true;
      break;
    case 0x18: // FlushQ
      Opcode = TOpcode::Flush;
      Flags.Quiet = true;
      break;
    case 0x19: // AppendQ
      Opcode = TOpcode::Append;
      Flags.Quiet = true;
      break;
    case 0x1A: // PrependQ
      Opcode = TOpcode::Prepend;
      Flags.Quiet = true;
      break;
    default:
      throw std::invalid_argument("Illegal opcode");
  }

  // Make sure we only have a key if we're allowed to
  // And that it is of the right length
  if (Opcode == TOpcode::Quit || Opcode == TOpcode::Flush || Opcode == TOpcode::NoOp || Opcode == TOpcode::Version) {
    if (header.KeyLength) {
      throw std::invalid_argument("Specified operation doesn't take a key");
    }
  } else {
    if (header.KeyLength > 250) {
      throw std::invalid_argument("Key too long");
    }
  }


  // Check extras length (depends on request kind)
  if (Opcode == TOpcode::Get) {
    if (header.ExtrasLength != 4) {
      throw std::invalid_argument("Extras not the correct length (4 bytes)");
    }
  } else if (Opcode == TOpcode::Set || Opcode == TOpcode::Add || Opcode == TOpcode::Replace) {
    if (header.ExtrasLength != 8) {
      throw std::invalid_argument("Extras not the correct length (8 bytes)");
    }
  } else if (Opcode == TOpcode::Increment || Opcode == TOpcode::Decrement) {
    if (header.ExtrasLength != 20) {
      throw std::invalid_argument("Extras not the correct length (20 bytes)");
    }
  } else {
    if (header.ExtrasLength) {
      throw std::invalid_argument("Extras not the correct length (0 bytes)");
    }
  }

  if (header.DataType) {
    throw std::invalid_argument("Data type must be null");
  }

  if (header.Reserved) {
    throw std::invalid_argument("Reserved field must be null");
  }

  // Check total body length
  //NOTE: This guarantees we won't underflow in later calculations, as well as checking that the packet is well formed
  if (header.TotalBodyLength < header.ExtrasLength + header.KeyLength) {
    throw std::invalid_argument("Total body length is too short to contain key + extras");
  }

  uint32_t value_len = header.TotalBodyLength - header.ExtrasLength - header.KeyLength;

  // Only some operations allow a value
  if (Opcode == TOpcode::Set || Opcode == TOpcode::Add || Opcode == TOpcode::Replace || Opcode == TOpcode::Append ||
      Opcode == TOpcode::Prepend) {
    // TODO: append and prepend the max value length depends on how much is already in the value of what we're inserting to.

  } else {
    if (value_len != 0) {
      throw std::invalid_argument("Given opcode doesn't accept a value");
    }
  }

  // We don't look at or touch this field
  Opaque = header.Opaque;

  //TODO: This field can only get verified at db transaction time.
  Cas = header.Cas;

  // Read extras, key, and value. We've already proofed which are needed (and made sure they were provided).
  Extras.Fill(header.ExtrasLength, in);
  Key.Fill(header.KeyLength, in);
  Value.Fill(value_len, in);
}