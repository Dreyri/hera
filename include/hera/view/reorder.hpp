#pragma once

#include "hera/container/integer_sequence.hpp"
#include "hera/size.hpp"
#include "hera/view/all.hpp"
#include "hera/view/detail/closure.hpp"
#include "hera/view/interface.hpp"

namespace hera
{
template<hera::range V, std::size_t... Is> // clang-format off
    requires view<V> && (!hera::range_out_of_bounds<V, Is> && ...)
class reorder_view : public view_interface<reorder_view<V, Is...>> // clang-format on
{
private:
    [[no_unique_address]] V base_;

public:
    /// The sequence used for reordering
    static constexpr hera::index_sequence<Is...> sequence{};

public:
    constexpr reorder_view(V base) noexcept(
        std::is_nothrow_move_constructible_v<V>)
        : base_{std::move(base)}
    {}

    constexpr reorder_view(V base, hera::index_sequence<Is...>) noexcept(
        std::is_nothrow_move_constructible_v<V>)
        : reorder_view{std::move(base)}
    {}

    constexpr V base() const noexcept(std::is_nothrow_copy_constructible_v<V>)
    {
        return base_;
    }

    constexpr auto size() const noexcept
    {
        return hera::size(sequence);
    }

    template<std::size_t I> // clang-format off
        requires (I < sizeof...(Is))
    constexpr decltype(auto) get() const noexcept // clang-format on
    {
        constexpr std::size_t pos = decltype(hera::get<I>(sequence))::value;

        return hera::get<pos>(base_);
    }
};

template<hera::range R, std::size_t... Is>
reorder_view(R&&, hera::index_sequence<Is...>)
    ->reorder_view<hera::all_view<R>, Is...>;

namespace views
{
struct reorder_fn
{
    template<hera::range R, std::size_t... Is> // clang-format off
            requires viewable_range<R> // clang-format on
        constexpr auto operator()(R&&                         r,
                                  hera::index_sequence<Is...> indices) const
        noexcept(noexcept(hera::reorder_view{std::forward<R>(r),
                                             std::move(indices)}))
            -> decltype(hera::reorder_view{std::forward<R>(r),
                                           std::move(indices)})
    {
        return hera::reorder_view{std::forward<R>(r), std::move(indices)};
    }

    template<std::size_t... Is>
    constexpr auto operator()(hera::index_sequence<Is...> indices) const
        noexcept(noexcept(detail::view_closure{*this, std::move(indices)}))
    {
        return detail::view_closure{*this, std::move(indices)};
    }
};

inline constexpr auto reorder = reorder_fn{};
} // namespace views
} // namespace hera
