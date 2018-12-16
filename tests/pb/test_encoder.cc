
#include "tests/test_util.h"
#include "tests/upb_test.h"
#include "upb/bindings/stdc++/string.h"
#include "google/protobuf/descriptor.upb.h"
#include "upb/pb/decoder.h"
#include "upb/pb/encoder.h"

std::string read_string(const char *filename) {
  size_t len;
  char *str = upb_readfile(filename, &len);
  ASSERT(str);
  if (!str) { return std::string(); }
  std::string ret = std::string(str, len);
  free(str);
  return ret;
}

void test_pb_roundtrip() {
  std::string input = read_string("google/protobuf/descriptor.pb");
  upb::SymbolTable* symtab = upb::SymbolTable::New();
  upb::Arena arena;
  google_protobuf_FileDescriptorSet *set =
      google_protobuf_FileDescriptorSet_parsenew(
          upb_stringview_make(input.c_str(), input.size()), &arena);
  ASSERT(set);
  size_t n;
  const google_protobuf_FileDescriptorProto *const *files =
      google_protobuf_FileDescriptorSet_file(set, &n);
  ASSERT(n == 1);
  upb::Status status;
  bool ok = symtab->AddFile(files[0], &status);
  if (!ok) {
    fprintf(stderr, "Error building def: %s\n", upb_status_errmsg(&status));
    ASSERT(false);
  }
  const upb::MessageDef *md =
      symtab->LookupMessage("google.protobuf.FileDescriptorSet");
  ASSERT(md);
  printf("name: %s\n", md->full_name());
  upb::reffed_ptr<const upb::Handlers> encoder_handlers(
      upb::pb::Encoder::NewHandlers(md));
  upb::reffed_ptr<const upb::pb::DecoderMethod> method(
      upb::pb::DecoderMethod::New(
          upb::pb::DecoderMethodOptions(encoder_handlers.get())));

  upb::InlinedEnvironment<512> env;
  std::string output;
  upb::StringSink string_sink(&output);
  upb::pb::Encoder* encoder =
      upb::pb::Encoder::Create(&env, encoder_handlers.get(),
                               string_sink.input());
  upb::pb::Decoder* decoder =
      upb::pb::Decoder::Create(&env, method.get(), encoder->input());
  ok = upb::BufferSource::PutBuffer(input, decoder->input());
  ASSERT(ok);
  ASSERT(input == output);
  upb::SymbolTable::Free(symtab);
}

extern "C" {
int run_tests(int argc, char *argv[]) {
  UPB_UNUSED(argc);
  UPB_UNUSED(argv);
  test_pb_roundtrip();
  return 0;
}
}
