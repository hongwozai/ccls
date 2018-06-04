#include "diagnostics_publisher.hh"

#include "pipeline.hh"
using namespace ccls;

#include <chrono>

void DiagnosticsPublisher::Init() {
  frequencyMs_ = g_config->diagnostics.frequencyMs;
  match_ = std::make_unique<GroupMatch>(g_config->diagnostics.whitelist,
                                        g_config->diagnostics.blacklist);
}

void DiagnosticsPublisher::Publish(WorkingFiles* working_files,
                                   std::string path,
                                   std::vector<lsDiagnostic> diagnostics) {
  // Cache diagnostics so we can show fixits.
  working_files->DoActionOnFile(path, [&](WorkingFile* working_file) {
    if (working_file)
      working_file->diagnostics_ = diagnostics;
  });

  int64_t now =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::high_resolution_clock::now().time_since_epoch())
          .count();
  if (frequencyMs_ >= 0 && (nextPublish_ <= now || diagnostics.empty()) &&
      match_->IsMatch(path)) {
    nextPublish_ = now + frequencyMs_;

    Out_TextDocumentPublishDiagnostics out;
    out.params.uri = lsDocumentUri::FromPath(path);
    out.params.diagnostics = diagnostics;
    pipeline::WriteStdout(kMethodType_TextDocumentPublishDiagnostics, out);
  }
}
