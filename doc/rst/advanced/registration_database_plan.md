# eCAL Registration Database: Analysis and Implementation Plan

## Goal
Move from a "raw `eCAL::Registration::Sample` forwarded everywhere" model to a central `EcalRegistrationDatabase` model with:
- incremental updates from incoming external registrations,
- query APIs for gates / monitoring / tooling,
- diff support (new / removed / changed entities),
- low-contention concurrent reads,
- and a migration path that allows bypassing full `Sample` materialization for hot update paths.

---

## 1) Current sample-processing analysis

### 1.1 Ingress and fan-out path
Today, incoming registration samples follow this path:

1. Transport receiver (`CRegistrationReceiverUDP` / `CRegistrationReceiverSHM`) receives serialized sample(s).
2. `CRegistrationReceiver` forwards every accepted sample through `CSampleApplier`.
3. `CSampleApplier` applies filtering (same process / host / domain / network) and invokes all registered callbacks.
4. Current callbacks include:
   - `timeout` (`CTimeoutProvider`) for synthetic unregister generation,
   - `gates` (`CSampleApplierGates`) for pub/sub/service propagation,
   - `monitoring` (`CMonitoringImpl`) for monitoring state enrichment,
   - optional user callbacks (`custom_registration`).

### 1.2 Per-component sample handling today

- **Timeout provider** stores a sample-keyed expiration map and creates unregister samples on timeout.
- **DescGate** stores topic/service/client/server views and emits callbacks for new / deleted entities.
- **Monitoring** reconstructs and maintains process/topic/service/client maps directly from every sample.
- **Pub/Sub gates** do mostly immediate routing work when a registration/unregistration arrives.
- **RegistrationProvider** periodically builds a `SampleList` from process + all local gates and appends pending delta samples.

### 1.3 Pain points observed

- Same semantic state is reconstructed in multiple places from the same sample stream.
- Diff/event logic (new/deleted detection) is duplicated per subsystem.
- Entity lifetime (explicit unregister vs timeout) is spread across timeout + consumers.
- Query use-cases require bespoke maps per component.

---

## 2) Target architecture

Introduce a central **`EcalRegistrationDatabase`** that is the single source of truth for external registration state.

### 2.1 Responsibilities

- Accept registration deltas (`reg` / `unreg` / timeout-generated unreg).
- Maintain normalized current state by entity key.
- Provide strongly typed queries.
- Provide deterministic diff outputs between revisions/snapshots.
- Support lock-light concurrent readers (snapshot reads) and serialized writer updates.
- Allow a direct mutation API that can be fed from deserialization without constructing full `Registration::Sample` for every update.

### 2.2 Non-responsibilities

- Transport receive/send and wire serialization.
- Filtering policy from `CSampleApplier` (stays there initially).
- Local self-registration ownership (kept in local gates/provider in first phase).

---

## 3) Proposed API (revised draft)

### 3.1 Ingestion API (two levels)

```cpp
class EcalRegistrationDatabase
{
public:
  using Revision = uint64_t;

  struct ApplyResult
  {
    Revision new_revision;
    std::vector<EntityEvent> events; // new_entity, updated_entity, deleted_entity
  };

  // Compatibility / transition path
  ApplyResult ApplySample(const Registration::Sample& sample);
  ApplyResult ApplySamples(const Registration::SampleList& samples);

  // Preferred long-term mutation API (deserializer-friendly)
  ApplyResult AddOrUpdateProcess(const ProcessKey&, const ProcessRegistrationDelta&);
  ApplyResult RemoveProcess(const ProcessKey&); // cascades process members

  ApplyResult AddOrUpdatePublisher(const EntityKey&, const TopicRegistrationDelta&);
  ApplyResult UpdatePublisherMonitoring(const EntityKey&, const TopicMonitoringDelta&);
  ApplyResult RemovePublisher(const EntityKey&);

  ApplyResult AddOrUpdateSubscriber(const EntityKey&, const TopicRegistrationDelta&);
  ApplyResult UpdateSubscriberMonitoring(const EntityKey&, const TopicMonitoringDelta&);
  ApplyResult RemoveSubscriber(const EntityKey&);

  ApplyResult AddOrUpdateServer(const EntityKey&, const ServiceRegistrationDelta&);
  ApplyResult UpdateServerMonitoring(const EntityKey&, const ServiceMonitoringDelta&);
  ApplyResult RemoveServer(const EntityKey&);

  ApplyResult AddOrUpdateClient(const EntityKey&, const ServiceRegistrationDelta&);
  ApplyResult UpdateClientMonitoring(const EntityKey&, const ServiceMonitoringDelta&);
  ApplyResult RemoveClient(const EntityKey&);

  RegistrationSnapshot GetSnapshot() const;
  Revision CurrentRevision() const;
  Revision PreviousRevision() const; // only current+previous required in first implementation

  RegistrationDiff Diff(Revision from, Revision to) const;
};
```

### 3.2 Keying rules (updated)

- **Entity primary key:** `entity_id` (64-bit), as unique key in-system.
- **Process key:** `process_id` (plus host-scoping only if needed for cross-host merge views).
- `host_name` and `process_id` remain indexed metadata and validation context, but not required as the canonical entity key.

### 3.3 Normalization and updates

- Normalize legacy + modern datatype fields once on ingest.
- Separate registration updates from monitoring-only updates to avoid touching cold fields.
- Unregister missing key => no-op (idempotent).
- Process unregistration / timeout => **immediate cascade deletion** of all member entities.

### 3.4 Deserialization integration path

For performance-sensitive paths, deserialize directly into a compact "delta command" and call the direct mutation API:

```cpp
// pseudo
Deserializer::OnPublisherMonitoring(entity_id, dynamic_fields)
  -> db.UpdatePublisherMonitoring(entity_id, dynamic_fields);
```

This allows skipping full re-deserialization of immutable/seldom-changing fields.

---

## 4) Data layout and cache-locality strategy

To address hot vs cold data and cache locality concerns:

1. **Split stores by mutability class**
   - **Cold/mostly immutable registration store:** names, datatype info, static layer config, method signatures.
   - **Hot monitoring store:** clocks, frequencies, latency stats, connection counters, drop counters.
   - **Warm metadata/index store:** process membership, name-to-id indexes.

2. **Use SoA-like compact hot records where possible**
   - Keep high-frequency numeric monitoring fields densely packed (avoid string-heavy structs in hot path).

3. **Indirection only where it pays off**
   - Large immutable payloads (descriptor strings, method vectors) behind shared ownership (`shared_ptr<const T>`).
   - Keep hot structs pointer-light to reduce cache misses.

4. **Update granularity**
   - `AddOrUpdate*Registration(...)` mutates cold+index data.
   - `Update*Monitoring(...)` mutates only hot monitoring shard.

---

## 5) Unit test plan (GTest)

Create a new test target e.g. `registration_database_test`.

### 5.1 Core behavior tests

1. Insert and query publisher.
2. Update existing entity (same `entity_id`) with dynamic-field changes only.
3. Unregister removes entity.
4. Idempotent unregister unknown key.
5. Process unregistration cascades members (publisher/subscriber/server/client).

### 5.2 Diff/revision tests

6. Revision diff new/deleted.
7. Diff stability (no false positives on identical normalized state).
8. Revision retention policy: only `current` and `previous` are guaranteed.

### 5.3 Snapshot/concurrency tests

9. Concurrent readers + single writer (consistent snapshots).
10. Snapshot immutability.

### 5.4 Mutation-path tests

11. `ApplySample(...)` and direct `AddOrUpdate*/Remove*` produce equivalent state.
12. Monitoring-only update does not rewrite registration payload pointers.

---

## 6) Incremental refactor plan

### Phase 0: Discovery + hardening
- Add adapter to feed database in shadow mode.
- Add counters: samples applied, state size, events emitted, direct-mutation count.

### Phase 1: Introduce DB + tests
- Implement DB module in `core/src/registration/` with two-store split (registration + monitoring).
- Implement both ingestion surfaces (`ApplySample` and direct mutation API).

### Phase 2: Wire central ingest
- In receiver path, convert incoming data to DB updates.
- Keep sample-based callback path for parity until downstream migration is complete.

### Phase 3: Refactor consumers
- **DescGate** queries DB and subscribes to DB events.
- **Monitoring** consumes DB state directly (or separate monitoring DB if split remains beneficial).
- **Gates** consume DB diffs/events where suitable.

### Phase 4: Cleanup
- Remove duplicated per-component registration caches.
- Keep only component-private runtime state not represented in DB.

### Phase 5: Optimize
- Micro-benchmark deserialization + apply path (sample-based vs direct mutation).
- Tune hot/cold splitting and index strategy.

---

## 7) Internal data structure options (updated)

### Option A: Immutable persistent backend (immer)

- Best snapshot semantics and structural sharing.
- Useful baseline for correctness and API shape.

### Option B: STL/shared_ptr COW backend (preferred first production step)

- Keep immutable snapshot object behind `std::shared_ptr<const State>`.
- Writer builds next state with selective copy-on-write of changed shards.
- Replace immer containers with STL containers + shared ownership where beneficial.

### Option C: RW-lock mutable maps + journal

- Simpler but weaker snapshot behavior and more lock contention.

### Recommendation

- Start with **Option B** for lower dependency risk and explicit cache-layout control.
- Keep backend swappable so an immer-backed implementation can still be evaluated later.

---

## 8) Decisions captured from this review

1. Process unregister/timeout **shall cascade-delete** all member entities.
2. Primary entity key is **`entity_id`**.
3. DB revision retention target is **current + previous** (first implementation).
4. Monitoring data should be handled as a distinct hot path (same DB with split stores, or a dedicated companion monitoring DB if profiling favors it).
5. Direct DB mutation API is required to enable deserialize-and-apply optimization.

---

## 9) Suggested first implementation slice (next PR)

1. Introduce `EcalRegistrationDatabase` with:
   - entity_id keyed storage,
   - registration/monitoring store split,
   - current+previous revision tracking,
   - process cascade delete.
2. Implement direct mutation methods for publisher/subscriber/process + sample-compat adapters.
3. Add GTests for equivalence, cascade behavior, revision retention, and hot/cold update isolation.

This creates a concrete, testable baseline and directly addresses performance concerns around deserialization and cache locality.
