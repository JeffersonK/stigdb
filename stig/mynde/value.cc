/* <stig/mynde/value.cc>

   Defines the record bits for <stig/mynde/value.h>

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

#include <stig/mynde/value.h>

#include <stig/native/record.h>


RECORD_ELEM(Stig::Mynde::TValue, Stig::Native::TBlob, Value);
RECORD_ELEM(Stig::Mynde::TValue, uint32_t, Flags);
