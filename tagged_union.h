/**
 * @file tagged_union.h
 */

#pragma once

#include <meta>

/**
 * @brief Used to tag the none enum value for a tagged union.
 */
struct tagged_union_none_value_t {};

/**
 * @brief Typesafe union using an enum to track which type in the union is active.
 *
 * Fully supports RAII.
 */
template<typename UNION_T, typename ENUM_T>
class tagged_union_t
{
public:
inline									tagged_union_t			();
inline									~tagged_union_t			();

inline									tagged_union_t			(const tagged_union_t& other);
inline				tagged_union_t&		operator=				(const tagged_union_t& other);
inline									tagged_union_t			(tagged_union_t&& other);
inline				tagged_union_t&		operator=				(tagged_union_t&& other);

inline				void				set						(const ENUM_T value);
inline				void				reset					();

					template<typename MEMBER_T>
inline				MEMBER_T*			get						();

					template<typename MEMBER_T>
inline				const MEMBER_T*		get						() const;

private:
static	consteval	size_t				calculate_union_size	();

					template<typename MEMBER_T>
inline	consteval	ENUM_T				get_enum_for_type		() const;

inline	consteval	ENUM_T				get_none_enum_value		() const;

					template<typename FN_T>
inline	constexpr	void				execute_for_enum		(char* const other_blob, const ENUM_T value, const FN_T& functor);
					template<typename FN_T>
inline	constexpr	void				execute_for_enum_const	(const char* const other_blob, const ENUM_T value, const FN_T& functor);

inline	constexpr	void				construct_for_enum		(const ENUM_T value);
inline	constexpr	void				destruct_for_enum		(const ENUM_T value);
inline	constexpr	void				copy_construct_for_enum	(const char* const other_blob, const ENUM_T value);
inline	constexpr	void				move_construct_for_enum	(char* const other_blob, const ENUM_T value);

	static constexpr size_t BLOB_SIZE = calculate_union_size();

	ENUM_T		m_value;
	char		m_blob[BLOB_SIZE];
};

/**
 * @brief Constructor.
 */
template<typename UNION_T, typename ENUM_T>
tagged_union_t<UNION_T, ENUM_T>::tagged_union_t()
	: m_value(get_none_enum_value())
	, m_blob()
{
}

/**
 * @brief Destructor.
 */
template<typename UNION_T, typename ENUM_T>
tagged_union_t<UNION_T, ENUM_T>::~tagged_union_t()
{
	reset();
}

/**
 * @brief Copy constructor.
 *
 * @param other
 */
template<typename UNION_T, typename ENUM_T>
tagged_union_t<UNION_T, ENUM_T>::tagged_union_t(const tagged_union_t& other)
	: tagged_union_t()
{
	m_value = other.m_value;
	copy_construct_for_enum(other.m_blob, m_value);
}

/**
 * @brief Copy asignment operator.
 *
 * @param other
 */
template<typename UNION_T, typename ENUM_T>
tagged_union_t<UNION_T, ENUM_T>& tagged_union_t<UNION_T, ENUM_T>::operator=(const tagged_union_t& other)
{
	reset();
	m_value = other.m_value;
	copy_construct_for_enum(other.m_blob, m_value);
	return *this;
}

/**
 * @brief Move constructor.
 *
 * @param other
 */
template<typename UNION_T, typename ENUM_T>
tagged_union_t<UNION_T, ENUM_T>::tagged_union_t(tagged_union_t&& other)
	: tagged_union_t()
{
	m_value = other.m_value;
	move_construct_for_enum(other.m_blob, m_value);
}

/**
 * @brief Move asignment operator.
 *
 * @param other
 */
template<typename UNION_T, typename ENUM_T>
tagged_union_t<UNION_T, ENUM_T>& tagged_union_t<UNION_T, ENUM_T>::operator=(tagged_union_t&& other)
{
	reset();
	m_value = other.m_value;
	move_construct_for_enum(other.m_blob, m_value);
	return *this;
}

/**
 * @brief Set the current enum value and destruct/construct the corresponding type.
 *
 * @param value
 */
template<typename UNION_T, typename ENUM_T>
void tagged_union_t<UNION_T, ENUM_T>::set(const ENUM_T value)
{
	if (m_value == value) {
		// No change
		return;
	}
	else if (m_value == get_none_enum_value()) {
		// From none to non-none
		m_value = value;
		construct_for_enum(m_value);
	}
	else if (value == get_none_enum_value()) {
		// From non-none to none
		destruct_for_enum(m_value);
		m_value = value;
	}
	else {
		// From non-none to non-none
		destruct_for_enum(m_value);
		m_value = value;
		construct_for_enum(m_value);
	}
}

/**
 * @brief Reset value to none.
 */
template<typename UNION_T, typename ENUM_T>
void tagged_union_t<UNION_T, ENUM_T>::reset()
{
	set(get_none_enum_value());
}

/**
 * @brief Try to get value as @a MEMBER_T. Return @c nullptr if value is of different type or none.
 */
template<typename UNION_T, typename ENUM_T>
template<typename MEMBER_T>
MEMBER_T* tagged_union_t<UNION_T, ENUM_T>::get()
{
	constexpr ENUM_T value = get_enum_for_type<MEMBER_T>();
	if (value != m_value) {
		return nullptr;
	}
	else {
		return reinterpret_cast<MEMBER_T*>(m_blob);
	}
}

/**
 * @brief Try to get value as @a MEMBER_T. Return @c nullptr if value is of different type or none.
 */
template<typename UNION_T, typename ENUM_T>
template<typename MEMBER_T>
const MEMBER_T* tagged_union_t<UNION_T, ENUM_T>::get() const
{
	constexpr ENUM_T value = get_enum_for_type<MEMBER_T>();
	if (value != m_value) {
		return nullptr;
	}
	else {
		return reinterpret_cast<const MEMBER_T*>(m_blob);
	}
}

/**
 * @brief Calculates the size of the largest type in the union.
 *
 * The total size of the union is sizeof(ENUM_T) + calculate_union_size().
 */
template<typename UNION_T, typename ENUM_T>
consteval size_t tagged_union_t<UNION_T, ENUM_T>::calculate_union_size()
{
	constexpr auto ctx = std::meta::access_context::unchecked();

	r_static_assert(std::meta::members_of(^^UNION_T, ctx).size() != 0, "");

	size_t size = 0;

	for (const auto& member : std::meta::members_of(^^UNION_T, ctx)) {
		if (std::meta::is_type(member)) {
			size = r_max(size, std::meta::size_of(member));
		}
	}

	return size;
}

/**
 * @brief Get enum @a MEMBER_T is annotated with.
 */
template<typename UNION_T, typename ENUM_T>
template<typename MEMBER_T>
consteval ENUM_T tagged_union_t<UNION_T, ENUM_T>::get_enum_for_type() const
{
	constexpr auto ctx = std::meta::access_context::unchecked();

	for (const auto& member : std::meta::members_of(^^UNION_T, ctx)) {
		if (std::meta::is_type(member)) {
			if (^^MEMBER_T == member) {
				return std::meta::extract<ENUM_T>(std::meta::annotations_of_with_type(member, ^^ENUM_T)[0]);
			}
		}
	}

	std::unreachable();
}

/**
 * @brief Get enum value annotated as none using @c tagged_union_none_value_t.
 */
template<typename UNION_T, typename ENUM_T>
consteval ENUM_T tagged_union_t<UNION_T, ENUM_T>::get_none_enum_value() const
{
	for (const auto& member : std::meta::enumerators_of(^^ENUM_T)) {
		if (std::meta::annotations_of_with_type(member, ^^tagged_union_none_value_t).size() != 0) {
			return std::meta::extract<ENUM_T>(member);
		}
	}

	std::unreachable();
}

/**
 * @brief Execute @a functor for current type.
 *
 * @param other_blob
 * @param value
 * @param functor
 */
template<typename UNION_T, typename ENUM_T>
template<typename FN_T>
constexpr void tagged_union_t<UNION_T, ENUM_T>::execute_for_enum(char* const other_blob, const ENUM_T value, const FN_T& functor)
{
	if (value == get_none_enum_value()) {
		return;
	}

	constexpr auto ctx = std::meta::access_context::unchecked();

	static constexpr auto members = std::define_static_array(std::meta::members_of(^^UNION_T, ctx));

	template for (constexpr auto& member : members) {
		if constexpr(std::meta::is_type(member)) {
			static constexpr auto annotations = std::define_static_array(std::meta::annotations_of_with_type(member, ^^ENUM_T));
			r_static_assert(annotations.size() != 0, "");
			const ENUM_T annotation = std::meta::extract<ENUM_T>(annotations[0]);

			if (annotation == value) {
				using type_t = [:member:];
				functor.template operator()<type_t>(m_blob, other_blob);
				return;
			}
		}
	}

	std::unreachable();
}

/**
 * @brief Execute @a functor for current type.
 *
 * @param other_blob
 * @param value
 * @param functor
 */
template<typename UNION_T, typename ENUM_T>
template<typename FN_T>
constexpr void tagged_union_t<UNION_T, ENUM_T>::execute_for_enum_const(const char* const other_blob, const ENUM_T value, const FN_T& functor)
{
	if (value == get_none_enum_value()) {
		return;
	}

	constexpr auto ctx = std::meta::access_context::unchecked();

	static constexpr auto members = std::define_static_array(std::meta::members_of(^^UNION_T, ctx));

	template for (constexpr auto& member : members) {
		if constexpr(std::meta::is_type(member)) {
			static constexpr auto annotations = std::define_static_array(std::meta::annotations_of_with_type(member, ^^ENUM_T));
			r_static_assert(annotations.size() != 0, "");
			const ENUM_T annotation = std::meta::extract<ENUM_T>(annotations[0]);

			if (annotation == value) {
				using type_t = [:member:];
				functor.template operator()<type_t>(m_blob, other_blob);
				return;
			}
		}
	}

	std::unreachable();
}

/**
 * @brief Construct @a m_blob as type corresponding to @a value.
 *
 * @param value
 */
template<typename UNION_T, typename ENUM_T>
constexpr void tagged_union_t<UNION_T, ENUM_T>::construct_for_enum(const ENUM_T value)
{
	const auto construct_fn = []<typename TYPE_T>(char* const blob, char* const other_blob) {
		UNREFERENCED(other_blob);
		new(blob) TYPE_T();
	};

	execute_for_enum(nullptr, value, construct_fn);
}

/**
 * @brief Destruct @a m_blob as type corresponding to @a value.
 *
 * @param value
 */
template<typename UNION_T, typename ENUM_T>
constexpr void tagged_union_t<UNION_T, ENUM_T>::destruct_for_enum(const ENUM_T value)
{
	const auto destruct_fn = []<typename TYPE_T>(char* const blob, char* const other_blob) {
		UNREFERENCED(other_blob);
		reinterpret_cast<TYPE_T*>(blob)->~TYPE_T();
	};

	execute_for_enum(nullptr, value, destruct_fn);
}

/**
 * @brief Copy construct @a m_blob from @a other_blob as type corresponding to @a value.
 *
 * @param other_blob
 * @param value
 */
template<typename UNION_T, typename ENUM_T>
constexpr void tagged_union_t<UNION_T, ENUM_T>::copy_construct_for_enum(const char* const other_blob, const ENUM_T value)
{
	const auto destruct_fn = []<typename TYPE_T>(char* const blob, const char* const other_blob) {
		new(blob) TYPE_T(*reinterpret_cast<const TYPE_T*>(other_blob));
	};

	execute_for_enum_const(other_blob, value, destruct_fn);
}

/**
 * @brief Move construct @a m_blob from @a other_blob as type corresponding to @a value.
 *
 * @param other_blob
 * @param value
 */
template<typename UNION_T, typename ENUM_T>
constexpr void tagged_union_t<UNION_T, ENUM_T>::move_construct_for_enum(char* const other_blob, const ENUM_T value)
{
	const auto destruct_fn = []<typename TYPE_T>(char* const blob, char* const other_blob) {
		new(blob) TYPE_T(std::move(*reinterpret_cast<TYPE_T*>(other_blob)));
	};

	execute_for_enum(other_blob, value, destruct_fn);
}
