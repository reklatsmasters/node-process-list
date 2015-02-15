#include <node.h>
#include <nan.h>
#include "snapshot.h"
#include "tasklist.h"

#include <algorithm>
#include <memory>

using namespace v8;

NAN_METHOD(snapshot_sync) {
	NanScope();

	// get list running processes
	// TODO: exceptions
	std::vector< std::shared_ptr<Process> > snap = tasklist ();
	Local<Array> tasks = NanNew<Array>(snap.size());

	bool verbose = args[0]->ToBoolean()->Value();

	for (uint32_t i = 0; i < tasks->Length(); ++i) {
		if (verbose) {
			Local<Object> hash = NanNew<Object>();

			hash->Set(
				NanNew<String>("name"),
				NanNew<String>( snap.at(i)->name().c_str() )
			);

			hash->Set(
				NanNew<String>("pid"),
				NanNew<Number>( snap.at(i)->pid() )
			);

			hash->Set(
				NanNew<String>("ppid"),
				NanNew<Number>( snap.at(i)->parentPid() )
			);

			hash->Set(
				NanNew<String>("path"),
				NanNew<String>( snap.at(i)->path().c_str() )
			);

			hash->Set(
				NanNew<String>("threads"),
				NanNew<Number>( snap.at(i)->threads() )
			);

			hash->Set(
				NanNew<String>("owner"),
				NanNew<String>( snap.at(i)->owner().c_str() )
			);

			hash->Set(
				NanNew<String>("priority"),
				NanNew<Number>( snap.at(i)->priority() )
			);

			tasks->Set(i, hash);
		} else {
			tasks->Set(i, NanNew<String>( snap.at(i)->name().c_str() ));
		}
	}

	NanReturnValue(tasks);
}

class SnapshotWorker : public NanAsyncWorker {
public:
	SnapshotWorker(NanCallback *callback, bool _verbose = false)
	: NanAsyncWorker(callback)
	{
		verbose = _verbose;
	}

	~SnapshotWorker(){};

	void Execute () {
		tasks = tasklist ();
	}

	void HandleOKCallback () {
		NanScope();

		Local<Array> jobs = NanNew<Array>(tasks.size());

		for (uint32_t i = 0; i < jobs->Length(); ++i) {
			if (verbose) {
				Local<Object> hash = NanNew<Object>();

				hash->Set(
					NanNew<String>("name"),
					NanNew<String>( tasks.at(i)->name().c_str() )
				);

				hash->Set(
					NanNew<String>("pid"),
					NanNew<Number>( tasks.at(i)->pid() )
				);

				hash->Set(
					NanNew<String>("ppid"),
					NanNew<Number>( tasks.at(i)->parentPid() )
				);

				hash->Set(
					NanNew<String>("path"),
					NanNew<String>( tasks.at(i)->path().c_str() )
				);

				hash->Set(
					NanNew<String>("threads"),
					NanNew<Number>( tasks.at(i)->threads() )
				);

				hash->Set(
					NanNew<String>("owner"),
					NanNew<String>( tasks.at(i)->owner().c_str() )
				);

				hash->Set(
					NanNew<String>("priority"),
					NanNew<Number>( tasks.at(i)->priority() )
				);

				jobs->Set(i, hash);
			} else {
				jobs->Set(i, NanNew<String>( tasks.at(i)->name().c_str() ));
			}
		}

		Local<Value> argv[] = {
			NanNull(),
			jobs
		};

		callback->Call(2, argv);
	}

private:
	std::vector< std::shared_ptr<Process> > tasks;
	bool verbose;
};

NAN_METHOD(snapshot_async) {
	NanScope();

	bool verbose = args[0]->ToBoolean()->Value();
	NanCallback *callback = new NanCallback(args[1].As<Function>());

	NanAsyncQueueWorker(new SnapshotWorker(callback, verbose));
	NanReturnUndefined();
}
