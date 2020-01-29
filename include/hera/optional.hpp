#pragma once

#include <type_traits>

#include "hera/concepts.hpp"

namespace hera
{
struct nullopt_t
{};

inline constexpr auto nullopt = nullopt_t{};

/// \cond
namespace detail
{
template<typename T>
struct const_ref
{
    using type = const T&;
};

template<>
struct const_ref<void>
{
    using type = void;
};

template<typename T>
using const_ref_t = typename const_ref<T>::type;

template<typename T>
struct just_storage_base;

template<typename T> // clang-format off
    requires (!std::is_reference_v<T>)
struct just_storage_base<T> // clang-format on
{
    using value_type = T;

    [[no_unique_address]] T val_;

    template<typename... Args>
    explicit constexpr just_storage_base(std::in_place_t, Args&&... args)
        : val_(static_cast<Args&&>(args)...)
    {}

    constexpr value_type& get() & noexcept
    {
        return val_;
    }

    constexpr const value_type& get() const& noexcept
    {
        return val_;
    }

    constexpr value_type&& get() && noexcept
    {
        return std::move(val_);
    }

    constexpr const value_type&& get() const&& noexcept
    {
        return std::move(val_);
    }
};

template<typename T> // clang-format off
    requires std::is_reference_v<T>
struct just_storage_base<T> // clang-format on
{
    using value_type = T;
    using raw_type   = std::remove_reference_t<T>;

    raw_type* val_;

    template<typename U>
    explicit constexpr just_storage_base(std::in_place_t, U&& u) noexcept
        : val_(std::addressof(u))
    {}

    constexpr value_type& get() & noexcept
    {
        return *val_;
    }

    constexpr const value_type& get() const& noexcept
    {
        return *val_;
    }

    constexpr value_type&& get() && noexcept
    {
        return static_cast<value_type&&>(*val_);
    }

    constexpr const value_type&& get() const&& noexcept
    {
        return static_cast<const value_type&&>(*val_);
    }
};

template<>
struct just_storage_base<void>
{
    just_storage_base() = default;

    constexpr just_storage_base(std::in_place_t) noexcept
    {}
};
} // namespace detail
/// \endcond

template<typename T>
class just;

class none;

template<typename Opt>
concept optional =
    hera::specialization_of<Opt, hera::just> || hera::same_as<Opt, hera::none>;

/// \brief A compile time optional value holding an item.
///
/// `just` represents an optional which is known at compile time to hold a
/// value.
/// \see none
template<typename T>
class just : private detail::just_storage_base<T>
{
private:
    using base = detail::just_storage_base<T>;

public:
    /// \brief the type of value we're holding
    using value_type = T;

public:
    /// \brief default constructs the held value.
    constexpr just() noexcept(std::is_nothrow_default_constructible_v<
                              T>) requires hera::constructible_from<T>
        : base(std::in_place)
    {}

    /// \brief construct held value in place from the given arguments.
    template<typename... Args>
    explicit constexpr just(std::in_place_t, Args&&... args)
        : base(std::in_place, static_cast<Args&&>(args)...)
    {}

    /// \brief construct held value from U&&
    template<typename U = value_type>
    constexpr just(U&& value) : base(std::in_place, static_cast<U&&>(value))
    {}

    /// \brief copy construct from another just
    ///
    /// This constructor is conditionally explicit if `const U&` is not
    /// convertible to T.
    template<typename U> // clang-format off
        requires (!hera::same_as<T, U>)
    explicit(!hera::convertible_to<const U&, T>) // clang-format on
        constexpr just(const just<U>& other)
        : just(*other)
    {}

    /// \brief move construct from another just
    ///
    /// This constructor is conditionally explicit if `U&&` is not convertible
    /// to T.
    template<typename U> // clang-format off
        requires (!hera::same_as<T, U>)
    explicit(!hera::convertible_to<U&&, T>) // clang-format on
        constexpr just(just<U>&& other)
        : just(*std::move(other))
    {}

    /// \brief checks whether `*this` contains a value
    ///
    /// Because this is known at compile time this method returns an instance of
    /// `std::true_type`.
    /// \return the boolean constant `std::true_type`.
    constexpr std::true_type has_value() const noexcept
    {
        return {};
    }

    /// \brief checks whether `*this` contains a value
    /// \return `true`
    explicit constexpr operator bool() const noexcept
    {
        return true;
    }

    /// \brief retrieve the inner value or an alternative.
    ///
    /// Because `just` always holds a value the given alternative will be
    /// ignored.
    /// \return The contained value as a const ref.
    template<typename U>
    constexpr detail::const_ref_t<T> value_or(U&&) const& noexcept
    {
        if constexpr (!hera::same_as<value_type, void>)
        {
            return **this;
        }
    }

    /// \cond
    template<typename U> // clang-format off
        requires (!hera::same_as<value_type, void>)
    constexpr T value_or(U&&) && // clang-format on
    {
        if constexpr (!hera::same_as<value_type, void>)
        {
            return **this;
        }
    }
    /// \endcond

    /// \brief retrieve the value contained within `*this`
    ///
    /// The method has overloads for all const/lvalue/rvalue qualifiers.
    /// \returns held value with forwarded qualifier
    constexpr value_type& operator*() &
        noexcept requires(!hera::same_as<void, T>)
    {
        return static_cast<value_type&>(this->get());
    }

    /// \cond
    constexpr const value_type&
    operator*() const& noexcept requires(!hera::same_as<void, T>)
    {
        return static_cast<const value_type&>(this->get());
    }

    constexpr value_type&& operator*() &&
        noexcept requires(!hera::same_as<void, T>)
    {
        return static_cast<value_type&&>(this->get());
    }

    constexpr const value_type&&
    operator*() const&& noexcept requires(!hera::same_as<void, T>)
    {
        return static_cast<const value_type&&>(this->get());
    }
    /// \endcond

private:
    template<typename F>
    constexpr auto transform_void(F&& fn) const requires hera::same_as<void, T>
    {
        static_assert(hera::invocable<F>, "cannot invoke F");

        using result = std::invoke_result_t<F>;

        if constexpr (hera::same_as<void, result>)
        {
            static_cast<F&&>(fn)();
            return hera::just<void>{std::in_place};
        }
        else
        {
            return hera::just<result>{static_cast<F&&>(fn)()};
        }
    }

public:
    /// \brief transform the contained value
    ///
    /// Apply `F` to the contained value and obtain an instance of `just`
    /// containing the returned value.
    /// If there is no value contained (in the case of `just<void>`), `F` will
    /// be invoked without arguments and its result wrapped in `just`.
    /// \return New `just` containing the result of `fn(**this)`.
    template<typename F>
    constexpr auto transform(F&& fn) &
    {
        if constexpr (hera::same_as<T, void>)
        {
            return transform_void(static_cast<F&&>(fn));
        }
        else
        {
            static_assert(hera::invocable<F, value_type&>, "cannot invoke F");

            using result = std::invoke_result_t<F, value_type&>;

            if constexpr (hera::same_as<void, result>)
            {
                static_cast<F&&>(fn)(**this);
                return hera::just<void>{std::in_place};
            }
            else
            {
                return hera::just<result>{static_cast<F&&>(fn)(**this)};
            }
        }
    }

    /// \cond
    template<typename F>
    constexpr auto transform(F&& fn) const&
    {
        if constexpr (hera::same_as<T, void>)
        {
            return transform_void(static_cast<F&&>(fn));
        }
        else
        {
            static_assert(hera::invocable<F, const value_type&>,
                          "cannot invoke F");

            using result = std::invoke_result_t<F, const value_type&>;

            if constexpr (hera::same_as<void, result>)
            {
                static_cast<F&&>(fn)(**this);
                return hera::just<void>{std::in_place};
            }
            else
            {
                return hera::just<result>{static_cast<F&&>(fn)(**this)};
            }
        }
    }

    template<typename F>
    constexpr auto transform(F&& fn) &&
    {
        if constexpr (hera::same_as<T, void>)
        {
            return transform_void(static_cast<F&&>(fn));
        }
        else
        {
            static_assert(hera::invocable<F, value_type&&>, "cannot invoke F");

            using result = decltype(static_cast<F&&>(fn)(*std::move(*this)));

            if constexpr (hera::same_as<void, result>)
            {
                static_cast<F&&>(fn)(*std::move(*this));
                return hera::just<void>{std::in_place};
            }
            else
            {
                return hera::just<result>{
                    static_cast<F&&>(fn)(*std::move(*this))};
            }
        }
    }

    template<typename F>
    constexpr auto transform(F&& fn) const&&
    {
        if constexpr (hera::same_as<T, void>)
        {
            return transform_void(static_cast<F&&>(fn));
        }
        else
        {
            static_assert(hera::invocable<F, const value_type&&>,
                          "cannot invoke F");

            using result = std::invoke_result_t<F, const value_type&&>;

            if constexpr (hera::same_as<void, result>)
            {
                static_cast<F&&>(fn)(*std::move(*this));
                return hera::just<void>{std::in_place};
            }
            else
            {
                return hera::just<result>{
                    static_cast<F&&>(fn)(*std::move(*this))};
            }
        }
    }
    /// \endcond

private:
    template<typename F>
    constexpr decltype(auto) and_then_void(F&& fn) const
    {
        static_assert(hera::invocable<F>, "cannot invoke F");
        static_assert(
            hera::optional<std::remove_cvref_t<std::invoke_result_t<F>>>,
            "F must return just/none");

        return static_cast<F&&>(fn)();
    }

public:
    /// \brief apply a function which returns another `optional`
    ///
    /// Unwraps the value inside and passes it to `fn`. The return value of `fn`
    /// must be another `optional`.
    ///
    /// There are additional overloads for const and rvalue qualifiers.
    /// \return the `optional` returned from invoke `fn(**this)`.
    template<typename F>
    constexpr decltype(auto) and_then(F&& fn) &
    {
        if constexpr (hera::same_as<void, T>)
        {
            return and_then_void(static_cast<F&&>(fn));
        }
        else
        {
            static_assert(hera::invocable<F, value_type&>, "cannot invoke F");
            static_assert(
                hera::optional<
                    std::remove_cvref_t<std::invoke_result_t<F, value_type&>>>,
                "F must return just/none");

            return static_cast<F&&>(fn)(**this);
        }
    }

    /// \cond
    template<typename F>
    constexpr decltype(auto) and_then(F&& fn) const&
    {
        if constexpr (hera::same_as<void, T>)
        {
            return and_then_void(static_cast<F&&>(fn));
        }
        else
        {
            static_assert(hera::invocable<F, const value_type&>,
                          "cannot invoke F");
            static_assert(hera::optional<std::remove_cvref_t<
                              std::invoke_result_t<F, const value_type&>>>,
                          "F must return just/none");

            return static_cast<F&&>(fn)(**this);
        }
    }

    template<typename F>
    constexpr decltype(auto) and_then(F&& fn) &&
    {
        if constexpr (hera::same_as<void, T>)
        {
            return and_then_void(static_cast<F&&>(fn));
        }
        else
        {
            static_assert(hera::invocable<F, value_type&&>, "cannot invoke F");
            static_assert(
                hera::optional<
                    std::remove_cvref_t<std::invoke_result_t<F, value_type&&>>>,
                "F must return just/none");

            return static_cast<F&&>(fn)(*std::move(*this));
        }
    }

    template<typename F>
    constexpr decltype(auto) and_then(F&& fn) const&&
    {
        if constexpr (hera::same_as<void, T>)
        {
            return and_then_void(static_cast<F&&>(fn));
        }
        else
        {
            static_assert(hera::invocable<F, const value_type&&>,
                          "cannot invoke F");
            static_assert(hera::optional<std::remove_cvref_t<
                              std::invoke_result_t<F, const value_type&&>>>,
                          "F must return just/none");

            return static_cast<F&&>(fn)(*std::move(*this));
        }
    }
    /// \endcond

    /// \brief gives the current optional if filled.
    ///
    /// Because `just` always holds a value, this simply gives `*this`.
    /// An rvalue qualified `just` will return `*this` with rvalue qualifiers.
    /// \returns `*this`
    template<typename F>
    constexpr const just& or_else(F&&) const& noexcept
    {
        return *this;
    }

    /// \cond
    template<typename F>
        constexpr just&& or_else(F&&) && noexcept
    {
        return std::move(*this);
    }
    /// \endcond

    /// \brief swap with another `just`
    constexpr void swap(just& other)
    {
        using std::swap;

        swap(this->val_, other.val_);
    }
};

just()->just<void>;

template<typename T>
just(T &&)->just<T>;

just(std::in_place_t)->just<void>;

class none
{
    using value_type = void;

public:
    none() = default;

    constexpr none(hera::nullopt_t) noexcept
    {}

    constexpr std::false_type has_value() const noexcept
    {
        return {};
    }

    explicit constexpr operator bool() const noexcept
    {
        return false;
    }

    template<typename U>
    constexpr U&& value_or(U&& u) const noexcept
    {
        return static_cast<U&&>(u);
    }

    template<typename F>
    constexpr hera::none transform(F&&) const noexcept
    {
        return {};
    }

    template<typename F>
    constexpr hera::none and_then(F&&) const noexcept
    {
        return {};
    }

    template<hera::invocable F>
    constexpr decltype(auto) or_else(F&& fn) const
        noexcept(std::is_nothrow_invocable_v<F>)
    {
        static_assert(
            hera::optional<std::remove_cvref_t<std::invoke_result_t<F>>>,
            "F must return just/none");

        return static_cast<F&&>(fn)();
    }
};
} // namespace hera