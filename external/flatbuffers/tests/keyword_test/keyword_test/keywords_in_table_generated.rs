// automatically generated by the FlatBuffers compiler, do not modify
// @generated
extern crate alloc;
extern crate flatbuffers;
use alloc::boxed::Box;
use alloc::string::{String, ToString};
use alloc::vec::Vec;
use core::mem;
use core::cmp::Ordering;
use self::flatbuffers::{EndianScalar, Follow};
use super::*;
pub enum KeywordsInTableOffset {}
#[derive(Copy, Clone, PartialEq)]

pub struct KeywordsInTable<'a> {
  pub _tab: flatbuffers::Table<'a>,
}

impl<'a> flatbuffers::Follow<'a> for KeywordsInTable<'a> {
  type Inner = KeywordsInTable<'a>;
  #[inline]
  unsafe fn follow(buf: &'a [u8], loc: usize) -> Self::Inner {
    Self { _tab: unsafe { flatbuffers::Table::new(buf, loc) } }
  }
}

impl<'a> KeywordsInTable<'a> {
  pub const VT_IS: flatbuffers::VOffsetT = 4;
  pub const VT_PRIVATE: flatbuffers::VOffsetT = 6;
  pub const VT_TYPE_: flatbuffers::VOffsetT = 8;
  pub const VT_DEFAULT: flatbuffers::VOffsetT = 10;

  pub const fn get_fully_qualified_name() -> &'static str {
    "KeywordTest.KeywordsInTable"
  }

  #[inline]
  pub unsafe fn init_from_table(table: flatbuffers::Table<'a>) -> Self {
    KeywordsInTable { _tab: table }
  }
  #[allow(unused_mut)]
  pub fn create<'bldr: 'args, 'args: 'mut_bldr, 'mut_bldr, A: flatbuffers::Allocator + 'bldr>(
    _fbb: &'mut_bldr mut flatbuffers::FlatBufferBuilder<'bldr, A>,
    args: &'args KeywordsInTableArgs
  ) -> flatbuffers::WIPOffset<KeywordsInTable<'bldr>> {
    let mut builder = KeywordsInTableBuilder::new(_fbb);
    builder.add_type_(args.type_);
    builder.add_private(args.private);
    builder.add_is(args.is);
    builder.add_default(args.default);
    builder.finish()
  }

  pub fn unpack(&self) -> KeywordsInTableT {
    let is = self.is();
    let private = self.private();
    let type_ = self.type_();
    let default = self.default();
    KeywordsInTableT {
      is,
      private,
      type_,
      default,
    }
  }

  #[inline]
  pub fn is(&self) -> ABC {
    // Safety:
    // Created from valid Table for this object
    // which contains a valid value in this slot
    unsafe { self._tab.get::<ABC>(KeywordsInTable::VT_IS, Some(ABC::void)).unwrap()}
  }
  #[inline]
  pub fn private(&self) -> public {
    // Safety:
    // Created from valid Table for this object
    // which contains a valid value in this slot
    unsafe { self._tab.get::<public>(KeywordsInTable::VT_PRIVATE, Some(public::NONE)).unwrap()}
  }
  #[inline]
  pub fn type_(&self) -> i32 {
    // Safety:
    // Created from valid Table for this object
    // which contains a valid value in this slot
    unsafe { self._tab.get::<i32>(KeywordsInTable::VT_TYPE_, Some(0)).unwrap()}
  }
  #[inline]
  pub fn default(&self) -> bool {
    // Safety:
    // Created from valid Table for this object
    // which contains a valid value in this slot
    unsafe { self._tab.get::<bool>(KeywordsInTable::VT_DEFAULT, Some(false)).unwrap()}
  }
}

impl flatbuffers::Verifiable for KeywordsInTable<'_> {
  #[inline]
  fn run_verifier(
    v: &mut flatbuffers::Verifier, pos: usize
  ) -> Result<(), flatbuffers::InvalidFlatbuffer> {
    use self::flatbuffers::Verifiable;
    v.visit_table(pos)?
     .visit_field::<ABC>("is", Self::VT_IS, false)?
     .visit_field::<public>("private", Self::VT_PRIVATE, false)?
     .visit_field::<i32>("type_", Self::VT_TYPE_, false)?
     .visit_field::<bool>("default", Self::VT_DEFAULT, false)?
     .finish();
    Ok(())
  }
}
pub struct KeywordsInTableArgs {
    pub is: ABC,
    pub private: public,
    pub type_: i32,
    pub default: bool,
}
impl<'a> Default for KeywordsInTableArgs {
  #[inline]
  fn default() -> Self {
    KeywordsInTableArgs {
      is: ABC::void,
      private: public::NONE,
      type_: 0,
      default: false,
    }
  }
}

pub struct KeywordsInTableBuilder<'a: 'b, 'b, A: flatbuffers::Allocator + 'a> {
  fbb_: &'b mut flatbuffers::FlatBufferBuilder<'a, A>,
  start_: flatbuffers::WIPOffset<flatbuffers::TableUnfinishedWIPOffset>,
}
impl<'a: 'b, 'b, A: flatbuffers::Allocator + 'a> KeywordsInTableBuilder<'a, 'b, A> {
  #[inline]
  pub fn add_is(&mut self, is: ABC) {
    self.fbb_.push_slot::<ABC>(KeywordsInTable::VT_IS, is, ABC::void);
  }
  #[inline]
  pub fn add_private(&mut self, private: public) {
    self.fbb_.push_slot::<public>(KeywordsInTable::VT_PRIVATE, private, public::NONE);
  }
  #[inline]
  pub fn add_type_(&mut self, type_: i32) {
    self.fbb_.push_slot::<i32>(KeywordsInTable::VT_TYPE_, type_, 0);
  }
  #[inline]
  pub fn add_default(&mut self, default: bool) {
    self.fbb_.push_slot::<bool>(KeywordsInTable::VT_DEFAULT, default, false);
  }
  #[inline]
  pub fn new(_fbb: &'b mut flatbuffers::FlatBufferBuilder<'a, A>) -> KeywordsInTableBuilder<'a, 'b, A> {
    let start = _fbb.start_table();
    KeywordsInTableBuilder {
      fbb_: _fbb,
      start_: start,
    }
  }
  #[inline]
  pub fn finish(self) -> flatbuffers::WIPOffset<KeywordsInTable<'a>> {
    let o = self.fbb_.end_table(self.start_);
    flatbuffers::WIPOffset::new(o.value())
  }
}

impl core::fmt::Debug for KeywordsInTable<'_> {
  fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
    let mut ds = f.debug_struct("KeywordsInTable");
      ds.field("is", &self.is());
      ds.field("private", &self.private());
      ds.field("type_", &self.type_());
      ds.field("default", &self.default());
      ds.finish()
  }
}
#[non_exhaustive]
#[derive(Debug, Clone, PartialEq)]
pub struct KeywordsInTableT {
  pub is: ABC,
  pub private: public,
  pub type_: i32,
  pub default: bool,
}
impl Default for KeywordsInTableT {
  fn default() -> Self {
    Self {
      is: ABC::void,
      private: public::NONE,
      type_: 0,
      default: false,
    }
  }
}
impl KeywordsInTableT {
  pub fn pack<'b, A: flatbuffers::Allocator + 'b>(
    &self,
    _fbb: &mut flatbuffers::FlatBufferBuilder<'b, A>
  ) -> flatbuffers::WIPOffset<KeywordsInTable<'b>> {
    let is = self.is;
    let private = self.private;
    let type_ = self.type_;
    let default = self.default;
    KeywordsInTable::create(_fbb, &KeywordsInTableArgs{
      is,
      private,
      type_,
      default,
    })
  }
}
