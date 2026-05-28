# Measurement API Reimplementation Plan (Python)

## Goals

1. Reimplement the Python measurement interface on top of the stable low-level `hdf5.py` wrapper (without changing `hdf5.py`).
2. Support both **reading** and **writing** measurements with serializer-driven behavior.
3. Use the serializer abstractions from `ecal.msg` (`common`, `proto`, `string`) instead of hardcoded protobuf-only behavior.
4. Add clear samples for:
   - binary measurement read/write,
   - protobuf measurement read/write,
   - string measurement read/write,
   following the naming and structure style used by existing Python communication samples.

---

## Phase 1: Baseline and API contract definition

1. Inventory current measurement APIs:
   - `measurement.py` (reader abstractions),
   - `writer.py` (writer abstractions),
   - current measurement samples.
2. Define a target public API that unifies reading and writing:
   - explicit `MeasurementReader` and `MeasurementWriter` entry points,
   - channel-level typed access based on serializers,
   - raw binary access for all channels.
3. Preserve compatibility where practical:
   - keep legacy class/function names as thin wrappers (deprecated) if needed,
   - avoid changing data layout or low-level HDF5 behavior.

Deliverable: short API sketch in docstrings / module comments before implementation.

---

## Phase 2: Serializer-driven type model for measurement

1. Introduce a measurement-local datatype info object compatible with `ecal.msg.common.serializer.DataTypeInfo` protocol:
   - fields: `name`, `encoding`, `descriptor`.
2. Implement robust conversion helpers:
   - parse channel type from HDF5 (`"encoding:type"`, fallback to empty encoding),
   - compose channel type string when writing,
   - map HDF5 channel metadata <-> serializer datatype info.
3. Provide a reusable serializer selection strategy:
   - caller passes a serializer instance explicitly for typed channels,
   - optional convenience constructors for protobuf / string.

Deliverable: internal helpers that isolate metadata parsing and serializer matching logic.

---

## Phase 3: Reading reimplementation

1. Build a new binary-first reader core on top of `hdf5.Meas`:
   - channel metadata object,
   - channel entry object with `rcv_timestamp`, `snd_timestamp`, `payload`.
2. Add typed read adapters:
   - `read_binary(...)` returns raw bytes,
   - `read_typed(serializer=...)` uses `accepts_data_with_type(...)` + `deserialize(...)`.
3. Support protobuf and string through msg serializers:
   - protobuf: `ecal.msg.proto.serializer.Serializer` / `DynamicSerializer`,
   - string: `ecal.msg.string.serializer.Serializer`.
4. Define explicit behavior for type mismatches and deserialization failures:
   - raise informative exceptions with channel + datatype context,
   - optionally provide iteration mode that reports and skips malformed entries.
5. Provide dynamic-channel support:
   - expose a `DynamicChannelReader` path that selects deserialization based on channel metadata,
   - for protobuf channels, use `ecal.msg.proto.serializer.DynamicSerializer` to return dynamically typed protobuf objects.

Deliverable: measurement reading path that no longer hardcodes protobuf logic in `measurement.py`.

---

## Phase 4: Writing reimplementation

1. Build writer channels that always store metadata from serializer datatype info:
   - `set_channel_type(encoding:type)` and `set_channel_description(descriptor)`.
2. Implement generic typed writer:
   - accepts serializer + message object,
   - serializes to bytes and writes timestamps.
3. Implement explicit binary writer:
   - accepts bytes payload directly,
   - optional metadata override (`encoding`, `type`, `descriptor`) for non-empty typed binary streams.
4. Add convenience factories:
   - protobuf writer channel from protobuf type,
   - string writer channel,
   - raw binary writer channel.
5. Ensure writer can create all three required sample formats (binary, protobuf, string).

Deliverable: `writer.py` no longer protobuf-only in `create_channel`; supports binary and string cleanly.

---

## Phase 5: Backward compatibility and migration

1. Keep old symbols where feasible and redirect to new internals.
2. Mark legacy-only APIs as deprecated in docstrings.
3. Document migration examples:
   - old protobuf writer usage -> new serializer-based usage.

Deliverable: existing users are not broken abruptly while new APIs are available.

---

## Phase 6: Samples (new high-level measurement examples)

Create six samples under `lang/python/samples/measurement/` using consistent naming:

1. `binary_write.py`
2. `binary_read.py`
3. `person_write.py` (protobuf)
4. `person_read.py` (protobuf)
5. `string_write.py`
6. `string_read.py`

Sample conventions:

- mirror style of `samples/core/pubsub/*_send.py` and `*_receive.py` naming,
- minimal dependencies and predictable output,
- configurable path to output measurement directory,
- comments that explain datatype metadata and serializer usage.

Also add/update sample README with run order and expected output.

Deliverable: discoverable, task-oriented examples for all requested formats.

---

## Phase 7: Test plan

1. Add or extend Python tests for measurement API:
   - roundtrip binary,
   - roundtrip protobuf,
   - roundtrip string,
   - serializer mismatch behavior,
   - metadata preservation (`encoding`, `name/type`, `descriptor`).
2. Keep tests deterministic (fixed timestamps / payloads).
3. Run targeted test subset and broader suite as available.

Deliverable: automated confidence for read/write and serializer integration.

---

## Phase 8: Rollout checklist

1. Verify imports and package paths for both `ecalhdf5` and `msg` modules.
2. Ensure no modifications to `hdf5.py`.
3. Validate sample scripts execute in sequence:
   - write -> read for each format.
4. Update changelog / release notes if required by repository practice.

Deliverable: complete feature with docs, samples, and tests ready for review.

---

## Suggested implementation order

1. Metadata helper + datatype info bridge.
2. New reader binary core and typed adapter.
3. New writer generic channel + binary/proto/string convenience.
4. Compatibility wrappers.
5. New samples + README updates.
6. Automated tests and final polish.
