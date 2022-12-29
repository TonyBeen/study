// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: msg.proto

#ifndef GOOGLE_PROTOBUF_INCLUDED_msg_2eproto
#define GOOGLE_PROTOBUF_INCLUDED_msg_2eproto

#include <limits>
#include <string>

#include <google/protobuf/port_def.inc>
#if PROTOBUF_VERSION < 3015000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers. Please update
#error your headers.
#endif
#if 3015005 < PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers. Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/port_undef.inc>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_table_driven.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/metadata_lite.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>  // IWYU pragma: export
#include <google/protobuf/extension_set.h>  // IWYU pragma: export
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>
#define PROTOBUF_INTERNAL_EXPORT_msg_2eproto
PROTOBUF_NAMESPACE_OPEN
namespace internal {
class AnyMetadata;
}  // namespace internal
PROTOBUF_NAMESPACE_CLOSE

// Internal implementation detail -- do not use these members.
struct TableStruct_msg_2eproto {
  static const ::PROTOBUF_NAMESPACE_ID::internal::ParseTableField entries[]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::AuxiliaryParseTableField aux[]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::ParseTable schema[1]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::FieldMetadata field_metadata[];
  static const ::PROTOBUF_NAMESPACE_ID::internal::SerializationTable serialization_table[];
  static const ::PROTOBUF_NAMESPACE_ID::uint32 offsets[];
};
extern const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_msg_2eproto;
::PROTOBUF_NAMESPACE_ID::Metadata descriptor_table_msg_2eproto_metadata_getter(int index);
namespace eular {
class ProtoBufTest;
struct ProtoBufTestDefaultTypeInternal;
extern ProtoBufTestDefaultTypeInternal _ProtoBufTest_default_instance_;
}  // namespace eular
PROTOBUF_NAMESPACE_OPEN
template<> ::eular::ProtoBufTest* Arena::CreateMaybeMessage<::eular::ProtoBufTest>(Arena*);
PROTOBUF_NAMESPACE_CLOSE
namespace eular {

// ===================================================================

class ProtoBufTest PROTOBUF_FINAL :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:eular.ProtoBufTest) */ {
 public:
  inline ProtoBufTest() : ProtoBufTest(nullptr) {}
  virtual ~ProtoBufTest();
  explicit constexpr ProtoBufTest(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  ProtoBufTest(const ProtoBufTest& from);
  ProtoBufTest(ProtoBufTest&& from) noexcept
    : ProtoBufTest() {
    *this = ::std::move(from);
  }

  inline ProtoBufTest& operator=(const ProtoBufTest& from) {
    CopyFrom(from);
    return *this;
  }
  inline ProtoBufTest& operator=(ProtoBufTest&& from) noexcept {
    if (GetArena() == from.GetArena()) {
      if (this != &from) InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  inline const ::PROTOBUF_NAMESPACE_ID::UnknownFieldSet& unknown_fields() const {
    return _internal_metadata_.unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(::PROTOBUF_NAMESPACE_ID::UnknownFieldSet::default_instance);
  }
  inline ::PROTOBUF_NAMESPACE_ID::UnknownFieldSet* mutable_unknown_fields() {
    return _internal_metadata_.mutable_unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
  }

  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* GetDescriptor() {
    return GetMetadataStatic().descriptor;
  }
  static const ::PROTOBUF_NAMESPACE_ID::Reflection* GetReflection() {
    return GetMetadataStatic().reflection;
  }
  static const ProtoBufTest& default_instance() {
    return *internal_default_instance();
  }
  static inline const ProtoBufTest* internal_default_instance() {
    return reinterpret_cast<const ProtoBufTest*>(
               &_ProtoBufTest_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    0;

  friend void swap(ProtoBufTest& a, ProtoBufTest& b) {
    a.Swap(&b);
  }
  inline void Swap(ProtoBufTest* other) {
    if (other == this) return;
    if (GetArena() == other->GetArena()) {
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(ProtoBufTest* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetArena() == other->GetArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  inline ProtoBufTest* New() const final {
    return CreateMaybeMessage<ProtoBufTest>(nullptr);
  }

  ProtoBufTest* New(::PROTOBUF_NAMESPACE_ID::Arena* arena) const final {
    return CreateMaybeMessage<ProtoBufTest>(arena);
  }
  void CopyFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) final;
  void MergeFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) final;
  void CopyFrom(const ProtoBufTest& from);
  void MergeFrom(const ProtoBufTest& from);
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) final;
  ::PROTOBUF_NAMESPACE_ID::uint8* _InternalSerialize(
      ::PROTOBUF_NAMESPACE_ID::uint8* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const final { return _cached_size_.Get(); }

  private:
  inline void SharedCtor();
  inline void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(ProtoBufTest* other);
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "eular.ProtoBufTest";
  }
  protected:
  explicit ProtoBufTest(::PROTOBUF_NAMESPACE_ID::Arena* arena);
  private:
  static void ArenaDtor(void* object);
  inline void RegisterArenaDtor(::PROTOBUF_NAMESPACE_ID::Arena* arena);
  public:

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;
  private:
  static ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadataStatic() {
    return ::descriptor_table_msg_2eproto_metadata_getter(kIndexInFileMessages);
  }

  public:

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kStrFieldNumber = 2,
    kIdFieldNumber = 1,
    kOptFieldNumber = 3,
  };
  // required string str = 2;
  bool has_str() const;
  private:
  bool _internal_has_str() const;
  public:
  void clear_str();
  const std::string& str() const;
  void set_str(const std::string& value);
  void set_str(std::string&& value);
  void set_str(const char* value);
  void set_str(const char* value, size_t size);
  std::string* mutable_str();
  std::string* release_str();
  void set_allocated_str(std::string* str);
  private:
  const std::string& _internal_str() const;
  void _internal_set_str(const std::string& value);
  std::string* _internal_mutable_str();
  public:

  // required int32 id = 1;
  bool has_id() const;
  private:
  bool _internal_has_id() const;
  public:
  void clear_id();
  ::PROTOBUF_NAMESPACE_ID::int32 id() const;
  void set_id(::PROTOBUF_NAMESPACE_ID::int32 value);
  private:
  ::PROTOBUF_NAMESPACE_ID::int32 _internal_id() const;
  void _internal_set_id(::PROTOBUF_NAMESPACE_ID::int32 value);
  public:

  // optional int32 opt = 3;
  bool has_opt() const;
  private:
  bool _internal_has_opt() const;
  public:
  void clear_opt();
  ::PROTOBUF_NAMESPACE_ID::int32 opt() const;
  void set_opt(::PROTOBUF_NAMESPACE_ID::int32 value);
  private:
  ::PROTOBUF_NAMESPACE_ID::int32 _internal_opt() const;
  void _internal_set_opt(::PROTOBUF_NAMESPACE_ID::int32 value);
  public:

  // @@protoc_insertion_point(class_scope:eular.ProtoBufTest)
 private:
  class _Internal;

  // helper for ByteSizeLong()
  size_t RequiredFieldsByteSizeFallback() const;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  ::PROTOBUF_NAMESPACE_ID::internal::HasBits<1> _has_bits_;
  mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr str_;
  ::PROTOBUF_NAMESPACE_ID::int32 id_;
  ::PROTOBUF_NAMESPACE_ID::int32 opt_;
  friend struct ::TableStruct_msg_2eproto;
};
// ===================================================================


// ===================================================================

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// ProtoBufTest

// required int32 id = 1;
inline bool ProtoBufTest::_internal_has_id() const {
  bool value = (_has_bits_[0] & 0x00000002u) != 0;
  return value;
}
inline bool ProtoBufTest::has_id() const {
  return _internal_has_id();
}
inline void ProtoBufTest::clear_id() {
  id_ = 0;
  _has_bits_[0] &= ~0x00000002u;
}
inline ::PROTOBUF_NAMESPACE_ID::int32 ProtoBufTest::_internal_id() const {
  return id_;
}
inline ::PROTOBUF_NAMESPACE_ID::int32 ProtoBufTest::id() const {
  // @@protoc_insertion_point(field_get:eular.ProtoBufTest.id)
  return _internal_id();
}
inline void ProtoBufTest::_internal_set_id(::PROTOBUF_NAMESPACE_ID::int32 value) {
  _has_bits_[0] |= 0x00000002u;
  id_ = value;
}
inline void ProtoBufTest::set_id(::PROTOBUF_NAMESPACE_ID::int32 value) {
  _internal_set_id(value);
  // @@protoc_insertion_point(field_set:eular.ProtoBufTest.id)
}

// required string str = 2;
inline bool ProtoBufTest::_internal_has_str() const {
  bool value = (_has_bits_[0] & 0x00000001u) != 0;
  return value;
}
inline bool ProtoBufTest::has_str() const {
  return _internal_has_str();
}
inline void ProtoBufTest::clear_str() {
  str_.ClearToEmpty();
  _has_bits_[0] &= ~0x00000001u;
}
inline const std::string& ProtoBufTest::str() const {
  // @@protoc_insertion_point(field_get:eular.ProtoBufTest.str)
  return _internal_str();
}
inline void ProtoBufTest::set_str(const std::string& value) {
  _internal_set_str(value);
  // @@protoc_insertion_point(field_set:eular.ProtoBufTest.str)
}
inline std::string* ProtoBufTest::mutable_str() {
  // @@protoc_insertion_point(field_mutable:eular.ProtoBufTest.str)
  return _internal_mutable_str();
}
inline const std::string& ProtoBufTest::_internal_str() const {
  return str_.Get();
}
inline void ProtoBufTest::_internal_set_str(const std::string& value) {
  _has_bits_[0] |= 0x00000001u;
  str_.Set(::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{}, value, GetArena());
}
inline void ProtoBufTest::set_str(std::string&& value) {
  _has_bits_[0] |= 0x00000001u;
  str_.Set(
    ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{}, ::std::move(value), GetArena());
  // @@protoc_insertion_point(field_set_rvalue:eular.ProtoBufTest.str)
}
inline void ProtoBufTest::set_str(const char* value) {
  GOOGLE_DCHECK(value != nullptr);
  _has_bits_[0] |= 0x00000001u;
  str_.Set(::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{}, ::std::string(value), GetArena());
  // @@protoc_insertion_point(field_set_char:eular.ProtoBufTest.str)
}
inline void ProtoBufTest::set_str(const char* value,
    size_t size) {
  _has_bits_[0] |= 0x00000001u;
  str_.Set(::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{}, ::std::string(
      reinterpret_cast<const char*>(value), size), GetArena());
  // @@protoc_insertion_point(field_set_pointer:eular.ProtoBufTest.str)
}
inline std::string* ProtoBufTest::_internal_mutable_str() {
  _has_bits_[0] |= 0x00000001u;
  return str_.Mutable(::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{}, GetArena());
}
inline std::string* ProtoBufTest::release_str() {
  // @@protoc_insertion_point(field_release:eular.ProtoBufTest.str)
  if (!_internal_has_str()) {
    return nullptr;
  }
  _has_bits_[0] &= ~0x00000001u;
  return str_.ReleaseNonDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), GetArena());
}
inline void ProtoBufTest::set_allocated_str(std::string* str) {
  if (str != nullptr) {
    _has_bits_[0] |= 0x00000001u;
  } else {
    _has_bits_[0] &= ~0x00000001u;
  }
  str_.SetAllocated(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), str,
      GetArena());
  // @@protoc_insertion_point(field_set_allocated:eular.ProtoBufTest.str)
}

// optional int32 opt = 3;
inline bool ProtoBufTest::_internal_has_opt() const {
  bool value = (_has_bits_[0] & 0x00000004u) != 0;
  return value;
}
inline bool ProtoBufTest::has_opt() const {
  return _internal_has_opt();
}
inline void ProtoBufTest::clear_opt() {
  opt_ = 0;
  _has_bits_[0] &= ~0x00000004u;
}
inline ::PROTOBUF_NAMESPACE_ID::int32 ProtoBufTest::_internal_opt() const {
  return opt_;
}
inline ::PROTOBUF_NAMESPACE_ID::int32 ProtoBufTest::opt() const {
  // @@protoc_insertion_point(field_get:eular.ProtoBufTest.opt)
  return _internal_opt();
}
inline void ProtoBufTest::_internal_set_opt(::PROTOBUF_NAMESPACE_ID::int32 value) {
  _has_bits_[0] |= 0x00000004u;
  opt_ = value;
}
inline void ProtoBufTest::set_opt(::PROTOBUF_NAMESPACE_ID::int32 value) {
  _internal_set_opt(value);
  // @@protoc_insertion_point(field_set:eular.ProtoBufTest.opt)
}

#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif  // __GNUC__

// @@protoc_insertion_point(namespace_scope)

}  // namespace eular

// @@protoc_insertion_point(global_scope)

#include <google/protobuf/port_undef.inc>
#endif  // GOOGLE_PROTOBUF_INCLUDED_GOOGLE_PROTOBUF_INCLUDED_msg_2eproto
