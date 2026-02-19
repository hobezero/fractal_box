#ifndef FRACTAL_BOX_RUNTIME_MESSAGE_MANAGER_HPP
#define FRACTAL_BOX_RUNTIME_MESSAGE_MANAGER_HPP

#include <new>
#include <type_traits>
#include <utility>
#include <vector>

#include "fractal_box/core/assert.hpp"
#include "fractal_box/core/containers/linear_flat_set.hpp"
#include "fractal_box/core/containers/type_object_map.hpp"
#include "fractal_box/core/init_tags.hpp"
#include "fractal_box/core/meta/meta.hpp"
#include "fractal_box/runtime/message_traits.hpp"

namespace fr {

struct TickPhaseTypeIdxDomain: DefaultTypeIndexDomain { };
using TickPhaseTypeIdx = TypeIndex<TickPhaseTypeIdxDomain>;

struct MessageTypeIdxDomain: CustomTypeIndexDomainBase<> {
	static constexpr auto null_value = std::numeric_limits<ValueType>::max();
};

using MessageTypeIdx = TypeIndex<MessageTypeIdxDomain>;

using MessageId = uint64_t;

template<class T>
inline constexpr auto is_message_or_list = false;

template<c_message T>
inline constexpr auto is_message_or_list<T> = true;

template<class... Ts>
inline constexpr auto is_message_or_list<MpList<Ts...>>
	= (is_message_or_list<Ts> && ... && true);

template<class T>
concept c_message_or_list = is_message_or_list<T>;

class MessageQueueBase {
public:
	template<class M>
	explicit constexpr
	MessageQueueBase(InPlaceAsInit<M>) noexcept:
		_tick_phase{TickPhaseTypeIdx::of<typename MessageTraits<M>::TickAt>}
	{ }

	auto consume_at(size_t idx) {
		_ttls[idx] = MessageTtl::Tombstone;
		--_count;
	}

	auto consume_interval(size_t start_idx, size_t end_idx) {
		FR_ASSERT(start_idx <= end_idx);
		FR_ASSERT(end_idx <= _ttls.size());
		for (auto i = start_idx; i < end_idx; ++i) {
			_ttls[i] = MessageTtl::Tombstone;
		}
		_count -= (end_idx - start_idx);
	}

	auto tick_phase() const noexcept -> TickPhaseTypeIdx { return _tick_phase; }

	auto ids() const noexcept -> const std::vector<MessageId>& {
		return _ids;
	}

	auto ttls() const noexcept -> const std::vector<MessageTtl>& {
		return _ttls;
	}

	[[nodiscard]]
	auto empty() const noexcept -> bool { return _ids.empty(); }

	auto size() const noexcept -> size_t { return _ids.size(); }

	auto alive_count() const noexcept -> size_t { return _count; }

protected:
	TickPhaseTypeIdx _tick_phase;
	MessageTtl _default_ttl;
	std::vector<MessageId> _ids;
	std::vector<MessageTtl> _ttls;
	size_t _count;
};

template<c_message M>
class MessageQueue: public MessageQueueBase {
public:
	constexpr
	MessageQueue() noexcept: MessageQueueBase{in_place_as<M>} { }

	template<class... Args>
	auto emplace_back_with_ttl(MessageId id, MessageTtl ttl, Args&&... args) {
		_messages.emplace_back(std::forward<Args>(args)...);
		_ids.push_back(id);
		_ttls.push_back(ttl);
	}

	void tick_unchecked(int count) noexcept {
		FR_ASSERT(_messages.size() == _ids.size() && _ids.size() == _ttls.size());
		FR_ASSERT(std::cmp_less_equal(count,
			std::numeric_limits<MessageTtl::ValueType>::max()));
		auto write_idx = 0uz;
		for (auto read_idx = 0uz; read_idx != _messages.size(); ++read_idx) {
			_ttls[read_idx] -= static_cast<MessageTtl::ValueType>(count);
			if (_ttls[read_idx].is_alive()) {
				_messages[write_idx] = std::move(_messages[read_idx]);
				_ids[write_idx] = _ids[read_idx];
				_ttls[write_idx] = _ttls[read_idx];
				++write_idx;
			}
		}
		using Diff = decltype(_ids)::difference_type;
		_messages.erase(_messages.begin() + static_cast<Diff>(write_idx), _messages.end());
		_ids.erase(_ids.begin() + static_cast<Diff>(write_idx), _ids.end());
		_ttls.erase(_ttls.begin() + static_cast<Diff>(write_idx), _ttls.end());
		_count = _ids.size();
	}

	void clear() noexcept {
		_messages.clear();
		_ids.clear();
		_ttls.clear();
	}

	auto messages() const noexcept -> const std::vector<M>& {
		return _messages;
	}

private:
	std::vector<M> _messages;
};

template<class T>
concept c_message_queue = c_specialization<T, MessageQueue>;

class AnyMessageQueue {
	struct DummyMessage {
		using TickAt = void;
	};
	using DummyQueue = MessageQueue<DummyMessage>;

	enum class Action {
		DestructiveMoveConstruct,
		DestructiveMoveAssign,
		Destroy,

		Tick,
		Clear,
	};

	struct Storage {
		explicit FR_FORCE_INLINE constexpr
		Storage(UninitializedInit) noexcept { }

		explicit FR_FORCE_INLINE constexpr
		Storage(ZeroInit) noexcept: buffer{} { }

	public:
		alignas(DummyQueue) unsigned char buffer[sizeof(DummyQueue)];
	};

	using HandlerFn = void (*)(Action action, AnyMessageQueue& dest, AnyMessageQueue* src, int in0);

	template<c_message_queue Q>
	struct Handler;

public:
	constexpr
	AnyMessageQueue() noexcept:
		_storage{zero_init},
		_handler{nullptr}
	{ }

	template<c_message M, class... Args>
	explicit constexpr
	AnyMessageQueue(InPlaceAsInit<M>, Args&&... ctor_args):
		_storage{uninitialized},
		_handler{&Handler<MessageQueue<M>>::handle}
	{
		Handler<MessageQueue<M>>::create(_storage, std::forward<Args>(ctor_args)...);
	}

	AnyMessageQueue(const AnyMessageQueue&) = delete;
	auto operator=(const AnyMessageQueue&) -> AnyMessageQueue& = delete;

	AnyMessageQueue(AnyMessageQueue&& other) noexcept:
		_storage{uninitialized}
	{
		if (other._handler) {
			other._handler(Action::DestructiveMoveConstruct, *this, &other, 0);
			_handler = std::exchange(other._handler, nullptr);
		}
		else {
			_handler = nullptr;
		}
	}

	auto operator=(AnyMessageQueue&& other) noexcept -> AnyMessageQueue& {
		if (this == &other)
			return *this;

		if (_handler && other._handler) {
			if (_handler == other._handler) {
				_handler(Action::DestructiveMoveAssign, *this, &other, 0);
				other._handler = nullptr;
			}
			else {
				_handler(Action::Destroy, *this, &other, 0);
				other._handler(Action::DestructiveMoveConstruct, *this, &other, 0);
				_handler = std::exchange(other._handler, nullptr);
			}
		}
		else if (_handler) {
			reset();
		}
		else if (other._handler) {
			other._handler(Action::DestructiveMoveConstruct, *this, &other, 0);
			_handler = std::exchange(other._handler, nullptr);
		}
		return *this;
	}

	~AnyMessageQueue() {
		if (_handler)
			_handler(Action::Destroy, *this, nullptr, 0);
	}

	void reset() noexcept {
		if (_handler) {
			_handler(Action::Destroy, *this, nullptr, 0);
			_handler = nullptr;
		}
	}

	template<class T>
	auto cast_unchecked() const noexcept -> const MessageQueue<T>& {
		FR_ASSERT(_handler == &Handler<MessageQueue<T>>::handle);
		return *Handler<MessageQueue<T>>::get(_storage);
	}

	template<class T>
	auto cast_unchecked() noexcept -> MessageQueue<T>& {
		FR_ASSERT(_handler == &Handler<MessageQueue<T>>::handle);
		return *Handler<MessageQueue<T>>::get(_storage);
	}

	void tick(TickPhaseTypeIdx tick_phase, int count) noexcept {
		FR_ASSERT(_handler);
		if (tick_phase == base_unchecked().tick_phase())
			_handler(Action::Tick, *this, nullptr, count);
	}

	void clear() noexcept {
		FR_ASSERT(_handler);
		_handler(Action::Clear, *this, nullptr, 0);
	}

	[[nodiscard]]
	auto empty() const noexcept -> bool { return !_handler || base_unchecked().empty(); }

	[[nodiscard]]
	auto empty() noexcept -> bool { return !_handler || base_unchecked().empty(); }

	auto size() const noexcept -> size_t { return _handler ? base_unchecked().size() : 0uz; }
	auto size() noexcept -> size_t { return _handler ? base_unchecked().size() : 0uz; }

	auto base_unchecked() const noexcept -> const MessageQueueBase& {
		FR_ASSERT_AUDIT(_handler);
		return *std::launder(static_cast<const MessageQueueBase*>(static_cast<const void*>(
			_storage.buffer)));
	}

	auto base_unchecked() noexcept -> MessageQueueBase& {
		FR_ASSERT_AUDIT(_handler);
		return *std::launder(static_cast<MessageQueueBase*>(static_cast<void*>(_storage.buffer)));
	}

	auto ids() const noexcept -> const std::vector<MessageId>& {
		FR_ASSERT_AUDIT(_handler);
		return base_unchecked().ids();
	}

	auto ttls() const noexcept -> const std::vector<MessageTtl>& {
		FR_ASSERT_AUDIT(_handler);
		return base_unchecked().ttls();
	}

private:
	Storage _storage;
	HandlerFn _handler;
};

template<c_message_queue Queue>
struct AnyMessageQueue::Handler {
	static_assert(sizeof(Storage) >= sizeof(Queue) && alignof(Storage) >= alignof(Queue));

	static FR_FORCE_INLINE constexpr
	auto get(Storage& storage) noexcept -> Queue* {
		return std::launder(static_cast<Queue*>(static_cast<void*>(storage.buffer)));
	}

	static FR_FORCE_INLINE constexpr
	auto get(const Storage& storage) noexcept -> const Queue* {
		return std::launder(static_cast<const Queue*>(static_cast<const void*>(storage.buffer)));
	}

	template<class... Args>
	static FR_FORCE_INLINE constexpr
	void create(Storage& storage, Args&&... args) {
		auto* p = static_cast<Queue*>(static_cast<void*>(&storage.buffer));
		std::construct_at(p, std::forward<Args>(args)...);
	}

	template<class U>
	static FR_FORCE_INLINE constexpr
	auto assign(Storage& storage, U&& other) {
		auto* p = std::launder(static_cast<Queue*>(static_cast<void*>(&storage.buffer)));
		*p = std::forward<U>(other);
	}

	template<class... Args>
	static FR_FORCE_INLINE constexpr
	void emplace(Storage& storage, Args&&... args) {
		std::destroy_at(get(storage));
		auto* p = static_cast<Queue*>(static_cast<void*>(&storage.buffer));
		std::construct_at(p, std::forward<Args>(args)...);
	}

	static FR_FORCE_INLINE constexpr
	void destroy(Storage& storage) {
		std::destroy_at(get(storage));
	}

	static
	void handle(Action action, AnyMessageQueue& dest, AnyMessageQueue* src, int in0) {
		using enum Action;
		auto& queue = *get(dest._storage);
		switch (action) {
			case DestructiveMoveConstruct: {
				FR_ASSERT_AUDIT(src);
				create(dest._storage, std::move(*get(src->_storage)));
				destroy(src->_storage);
				return;
			}
			case DestructiveMoveAssign: {
				FR_ASSERT_AUDIT(src && src->_handler == dest._handler && dest._handler ==
					&handle);
				assign(dest._storage, std::move(*get(src->_storage)));
				destroy(src->_storage);
				return;
			}
			case Destroy: {
				destroy(dest._storage);
				return;
			}
			case Tick: {
				queue.tick_unchecked(in0);
				return;
			}
			case Clear: {
				queue.clear();
				return;
			}
		}
		FR_UNREACHABLE();
	}
};

namespace detail {

using MessageQueueMap = TypeObjectMap<MessageTypeIdxDomain, AnyMessageQueue>;

/// @brief Current index/position of MessageReader
class MessagePosition {
public:
	MessagePosition(MessageQueueBase& base, size_t start, size_t end) noexcept:
		idx{start},
		end_idx{end}
	{
		while (this->idx < end && base.ttls()[this->idx].is_dead())
			++this->idx;
	}

	MessagePosition(MessageQueueBase& base, size_t start) noexcept:
		MessagePosition(base, start, base.size())
	{ }

	explicit FR_FORCE_INLINE
	operator bool() const noexcept { return this->idx < this->end_idx; }

public:
	size_t idx;
	// Store size beforehand in case someone tries to read and write messages of a single type
	// in the same system
	size_t end_idx;
};

class MessageCursor {
public:
	MessageCursor(MessagePosition& pos, MessageQueueBase& base) noexcept:
		_idx{&pos.idx},
		_end_idx{pos.end_idx},
		_base{&base}
	{ }

	explicit FR_FORCE_INLINE
	operator bool() const noexcept { return *_idx < _end_idx; }

	FR_FORCE_INLINE
	void operator++() noexcept {
		++*_idx;
		while (*_idx < _end_idx && _base->ttls()[*_idx].is_dead())
			++*_idx;
	}

	FR_FORCE_INLINE
	void consume() const noexcept { _base->consume_at(*_idx); }

	FR_FORCE_INLINE
	auto id() const noexcept -> MessageId { return _base->ids()[*_idx]; }

	FR_FORCE_INLINE
	auto ttl() const noexcept -> MessageTtl { return _base->ttls()[*_idx]; }

	FR_FORCE_INLINE
	auto index() const noexcept -> size_t { return *_idx; }

private:
	size_t* _idx;
	size_t _end_idx;
	MessageQueueBase* _base;
};

} // namespace detail

template<c_message... Ts>
class MessageWriter;

template<c_message... Ts>
class MessageReader {
	static constexpr auto ts_count = sizeof...(Ts);
	using Cursor = detail::MessageCursor;

public:
	using WriterType = MessageWriter<Ts...>;
	using Types = MpList<Ts...>;
	static constexpr auto types = mp_list<Ts...>;
	using HasRefSemantics = TrueC;

	explicit
	MessageReader(detail::MessageQueueMap& queues) noexcept:
		_queues{&queues},
		_positions{detail::MessagePosition{queues.get_unchecked<Ts>().base_unchecked(), 0}...}
	{ }

	template<class Sink>
	requires (ts_count == 1)
	auto for_one(Sink&& sink) -> size_t {
		using T = MpPackFirst<Ts...>;
		auto& queue = get_queue<T>();
		if (auto cursor = Cursor{_positions[0], queue}; cursor) {
			sink_message(sink, cursor, queue);
			++cursor;
			return 1;
		}
		return 0;
	}

	template<class Sink>
	requires (ts_count > 1)
	auto for_one(Sink&& sink) -> size_t {
		Cursor cursors[ts_count] = {
			{_positions[mp_find<Types, Ts>], get_queue<Ts>()}...
		};
		return for_one_impl(sink, cursors);
	}

	template<class Sink>
	requires (ts_count == 1)
	auto for_each(Sink&& sink) {
		using T = MpPackFirst<Ts...>;
		auto& queue = get_queue<T>();
		auto msg_count = 0uz;
		for (auto cursor = Cursor{_positions[0], queue}; cursor; ++cursor) {
			sink_message(sink, cursor, queue);
			++msg_count;
		}
		return msg_count;
	}

	template<class Sink>
	requires (ts_count > 1)
	auto for_each(Sink&& sink) -> size_t {
		Cursor cursors[ts_count] = {
			{_positions[mp_find<Types, Ts>], get_queue<Ts>()}...
		};
		auto msg_count = 0uz;
		while (for_one_impl(sink, cursors))
			++msg_count;
		return msg_count;
	}

	template<class Sink>
	auto for_each_consume(Sink&& sink) -> size_t {
		auto result = for_each(sink);
		consume_all();
		return result;
	}

	template<class Sink>
	requires (ts_count == 1)
	auto for_last(Sink&& sink) -> bool {
		using T = MpPackFirst<Ts...>;
		auto& queue = get_queue<T>();
		auto& pos = _positions[0];
		for (auto idx = pos.end_idx; idx-- > pos.idx;) {
			if (queue.ttls()[idx].is_alive()) {
				pos.idx = idx;
				auto cursor = Cursor{pos, queue};
				sink_message(sink, cursor, queue);
				pos.idx = pos.end_idx;
				return true;
			}
		}
		return false;
	}

	void reset() noexcept {
		[&]<size_t... Is>(std::index_sequence<Is...>) {
			((_positions[Is] = {get_queue<Ts>(), 0, _positions[Is].end_idx}), ...);
		}(std::make_index_sequence<ts_count>{});
	}

	void consume_all_pending() noexcept {
		[&]<size_t... Is>(std::index_sequence<Is...>) {
			(get_queue<Ts>().consume_interval(_positions[Is].idx, _positions[Is].end_idx), ...);
			((_positions[Is].idx = _positions[Is].end_idx), ...);
		}(std::make_index_sequence<ts_count>{});
	}

	void consume_all() noexcept {
		[&]<size_t... Is>(std::index_sequence<Is...>) {
			(get_queue<Ts>().consume_interval(0, _positions[Is].end_idx), ...);
			((_positions[Is].idx = _positions[Is].end_idx), ...);
		}(std::make_index_sequence<ts_count>{});
	}

	auto has_pending() const noexcept -> bool {
		for (auto pos : _positions) {
			if (pos)
				return true;
		}
		return false;
	}

	template<class T>
	requires c_mp_contains<Types, T>
	auto has_pending_of() const noexcept -> bool{
		return static_cast<bool>(_positions[mp_find<Types, T>]);
	}

private:
	template<class T>
	FR_FORCE_INLINE
	auto get_queue() const noexcept -> MessageQueue<T>& {
		return _queues->get_unchecked<T>().template cast_unchecked<T>();
	}

	template<class T>
	void sink_message(auto&& sink, Cursor& cursor, MessageQueue<T>& queue) {
		const auto& msg = queue.messages()[cursor.index()];
		using SinkResult = decltype(sink(msg));
		if constexpr (std::is_void_v<SinkResult>) {
			sink(msg);
		}
		else if constexpr (std::is_same_v<SinkResult, bool>) {
			if (sink(msg))
				cursor.consume();
		}
		else
			static_assert(always_false<T>, "Unsupported sink return type");
	}

	template<class Sink>
	auto for_one_impl(Sink&& sink, Cursor (&cursors)[ts_count]) -> size_t {
		// Basically a merge stage from merge sort over `ts_count` queues ordered by `MessageId`
		auto min_id = std::numeric_limits<MessageId>::max();
		auto min_j = ts_count;
		for (auto j = 0uz; j < ts_count; ++j) {
			if (!cursors[j])
				continue;
			const auto id = cursors[j].id();
			if (id < min_id) {
				min_id = id;
				min_j = j;
			}
		}
		if (min_j == ts_count)
			return 0;

		auto min_cursor = cursors[min_j];
		[&]<size_t... Is>(std::index_sequence<Is...>) {
			static_cast<void>((... || (Is == min_j
				? (sink_message(sink, min_cursor, get_queue<Ts>()), true) // Short-circuit
				: false
			)));
		}(std::make_index_sequence<ts_count>{});
		++min_cursor;

		return 1;
	}

private:
	// We save a pointer to the map because its address is guaranteed to be stable, unlike addresses
	// of the message queues
	detail::MessageQueueMap* _queues;
	// Can't store cursors directly since they also reference queues
	detail::MessagePosition _positions[ts_count];
};

template<class T>
concept c_message_reader = c_specialization<T, MessageReader>;

template<c_message_or_list... Ts>
using MessageListReader = MpRename<MpPackFlatten<MpList, Ts...>, MessageReader>;

template<c_message... Ts>
class MessageWriter {
	static constexpr auto ts_count = sizeof...(Ts);

public:
	using ReaderType = MessageReader<Ts...>;
	using Types = MpList<Ts...>;
	static constexpr auto types = mp_list<Ts...>;
	using HasRefSemantics = TrueC;

	explicit
	MessageWriter(detail::MessageQueueMap& queues, MessageId& next_id) noexcept:
		_queues{&queues},
		_next_id{&next_id}
	{ }

	template<class M, class... Args>
	requires c_mp_contains<Types, M>
	void emplace_as_with_ttl(MessageTtl ttl, Args&&... args) {
		auto& queue = _queues->get_unchecked<M>().template cast_unchecked<M>();
		queue.emplace_back_with_ttl((*_next_id)++, ttl, std::forward<Args>(args)...);
	}

	template<class T, class... Args>
	requires c_mp_contains<Types, T>
	FR_FORCE_INLINE
	void emplace_as(Args&&... args) {
		return emplace_as_with_ttl(MessageTraits<T>::default_ttl, std::forward<Args>(args)...);
	}

	template<class... Args>
	requires (ts_count == 1)
	FR_FORCE_INLINE
	void emplace_with_ttl(MessageTtl ttl, Args&&... args) {
		emplace_as_with_ttl<MpPackFirst<Ts...>>(ttl, std::forward<Args>(args)...);
	}

	template<class... Args>
	requires (ts_count == 1)
	FR_FORCE_INLINE
	void emplace(Args&&... args) {
		emplace_as<MpPackFirst<Ts...>>(std::forward<Args>(args)...);
	}

	template<class U>
	requires c_mp_contains<Types, std::remove_cvref_t<U>>
	FR_FORCE_INLINE
	void push(U&& message, MessageTtl ttl = MessageTraits<std::remove_cvref_t<U>>::default_ttl) {
		emplace_as_with_ttl<std::remove_cvref_t<U>>(ttl, std::forward<U>(message));
	}

private:
	detail::MessageQueueMap* _queues;
	MessageId* _next_id;
};

template<class T>
concept c_message_writer = c_specialization<T, MessageWriter>;

template<c_message_or_list... Ts>
using MessageListWriter = MpRename<MpPackFlatten<MpList, Ts...>, MessageWriter>;

/// @todo
///   TODO: Implement id normalization (reduce counters when we are about to overflow)
class MessageManager {
public:
	void register_tick_phase(TickPhaseTypeIdx tick_phase) {
		_tick_phases.insert(tick_phase);
	}

	template<class PhaseTag>
	void register_tick_phase() {
		_tick_phases.insert(TickPhaseTypeIdx::of<PhaseTag>);
	}

	template<c_not_mp_list... PhaseTags>
	void register_tick_phases() {
		(_tick_phases.insert(TickPhaseTypeIdx::of<PhaseTags>), ...);
	}

	template<c_mp_list PhaseList>
	void register_tick_phases() {
		[this]<class... PhaseTags>(MpList<PhaseTags...>) {
			(_tick_phases.insert(TickPhaseTypeIdx::of<PhaseTags>), ...);
		}(PhaseList{});
	}

	void tick(TickPhaseTypeIdx tick_phase, int count = 1) noexcept {
		FR_PANIC_CHECK(_tick_phases.contains(tick_phase));
		if (count == 0)
			return;
		for (auto&& [idx, queue] : _queues)
			queue.tick(tick_phase, count);
	}

	template<class PhaseTag>
	FR_FORCE_INLINE
	void tick(int count = 1) noexcept {
		tick(TickPhaseTypeIdx::of<PhaseTag>, count);
	}

	void clear() noexcept {
		for (auto&& [idx, queue] : _queues)
			queue.clear();
	}

	template<c_message... Ts>
	requires (!c_message_reader<Ts> && ...)
	auto make_reader() -> MessageReader<Ts...> {
		(_queues.try_emplace<Ts>(in_place_as<Ts>), ...);
		return MessageReader<Ts...>{_queues};
	}

	template<c_message... Ts>
	FR_FORCE_INLINE
	auto make_reader(MpList<Ts...>) -> MessageReader<Ts...> {
		return make_reader<Ts...>();
	}

	template<c_message_reader Reader>
	FR_FORCE_INLINE
	auto make_reader() -> Reader {
		return make_reader(Reader::types);
	}

	template<c_message... Ts>
	requires (!c_message_writer<Ts> && ...)
	auto make_writer() -> MessageWriter<Ts...> {
		(_queues.try_emplace<Ts>(in_place_as<Ts>), ...);
		return MessageWriter<Ts...>{_queues, _next_id};
	}

	template<c_message... Ts>
	FR_FORCE_INLINE
	auto make_writer(MpList<Ts...>) -> MessageWriter<Ts...> {
		return make_writer<Ts...>();
	}

	template<c_message_writer Writer>
	FR_FORCE_INLINE
	auto make_writer() -> Writer {
		return make_reader(Writer::types);
	}

	template<c_message M>
	auto get_queue_of() const noexcept -> const MessageQueue<M>& {
		return _queues.try_emplace<M>(in_place_as<M>).where->second.template cast_unchecked<M>();
	}

	template<c_message M>
	auto get_queue_of() noexcept -> MessageQueue<M>& {
		return _queues.try_emplace<M>(in_place_as<M>).where->second.template cast_unchecked<M>();
	}

private:
	TypeObjectMap<MessageTypeIdxDomain, AnyMessageQueue> _queues;
	MessageId _next_id = 0;
	LinearFlatSet<TickPhaseTypeIdx> _tick_phases;
};

} // namespace fr
#endif // include guard
