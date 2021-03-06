// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_NET_NODE_STREAM_LOADER_H_
#define SHELL_BROWSER_NET_NODE_STREAM_LOADER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/system/data_pipe_producer.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "v8/include/v8.h"

namespace electron {

// Read data from node Stream and feed it to NetworkService.
//
// This class manages its own lifetime and should delete itself when the
// connection is lost or finished.
//
// We use |paused mode| to read data from |Readable| stream, so we don't need to
// copy data from buffer and hold it in memory, and we only need to make sure
// the passed |Buffer| is alive while writing data to pipe.
class NodeStreamLoader : public network::mojom::URLLoader {
 public:
  NodeStreamLoader(network::ResourceResponseHead head,
                   network::mojom::URLLoaderRequest loader,
                   network::mojom::URLLoaderClientPtr client,
                   v8::Isolate* isolate,
                   v8::Local<v8::Object> emitter);

 private:
  ~NodeStreamLoader() override;

  using EventCallback = base::RepeatingCallback<void()>;

  void Start(network::ResourceResponseHead head);
  void NotifyComplete(int result);
  void ReadMore();
  void DidWrite(MojoResult result);

  // Subscribe to events of |emitter|.
  void On(const char* event, EventCallback callback);

  // URLLoader:
  void FollowRedirect(const std::vector<std::string>& removed_headers,
                      const net::HttpRequestHeaders& modified_headers,
                      const base::Optional<GURL>& new_url) override {}
  void SetPriority(net::RequestPriority priority,
                   int32_t intra_priority_value) override {}
  void PauseReadingBodyFromNet() override {}
  void ResumeReadingBodyFromNet() override {}

  mojo::Binding<network::mojom::URLLoader> binding_;
  network::mojom::URLLoaderClientPtr client_;

  v8::Isolate* isolate_;
  v8::Global<v8::Object> emitter_;
  v8::Global<v8::Value> buffer_;

  // Mojo data pipe where the data that is being read is written to.
  std::unique_ptr<mojo::DataPipeProducer> producer_;

  // Whether we are in the middle of write.
  bool is_writing_ = false;

  // When NotifyComplete is called while writing, we will save the result and
  // quit with it after the write is done.
  bool ended_ = false;
  int result_ = net::OK;

  // Store the V8 callbacks to unsubscribe them later.
  std::map<std::string, v8::Global<v8::Value>> handlers_;

  base::WeakPtrFactory<NodeStreamLoader> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(NodeStreamLoader);
};

}  // namespace electron

#endif  // SHELL_BROWSER_NET_NODE_STREAM_LOADER_H_
