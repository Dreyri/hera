#pragma once

#include "hera/ranges.hpp"
#include "hera/size.hpp"
#include "hera/view.hpp"
#include "hera/view/all.hpp"
#include "hera/view/detail/closure.hpp"
#include "hera/view/interface.hpp"

namespace hera
{
template<hera::range V> // clang-format off
    requires view<V> 
class move_view : public hera::view_interface<move_view<V>> // clang-format on
{
private:
    [[no_unique_address]] V base_;

public:
    constexpr move_view(V base) noexcept(
        std::is_nothrow_move_constructible_v<V>)
        : base_{std::move(base)}
    {}

    constexpr V base() const noexcept(std::is_nothrow_copy_constructible_v<V>)
    {
        return base_;
    }

    constexpr auto size() const noexcept
    {
        return hera::size(base_);
    }

    template<std::size_t I>
    constexpr auto try_at() const noexcept
    {
        return hera::try_at<I>(base_).transform(
            [](auto&& x) -> decltype(auto) { return std::move(x); });
    }
};

template<hera::range R>
move_view(R &&)->move_view<hera::all_view<R>>;

namespace views
{
struct move_fn : detail::pipeable_interface<move_fn>
{
    template<hera::range R>
    constexpr auto operator()(R&& r) const
        noexcept(noexcept(move_view(std::forward<R>(r))))
    {
        return move_view(std::forward<R>(r));
    }
};

constexpr auto move = move_fn{};
} // namespace views
} // namespace hera