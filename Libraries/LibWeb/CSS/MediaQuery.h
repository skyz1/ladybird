/*
 * Copyright (c) 2021-2025, Sam Atkins <sam@ladybird.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/FlyString.h>
#include <AK/NonnullRefPtr.h>
#include <AK/Optional.h>
#include <AK/OwnPtr.h>
#include <AK/RefCounted.h>
#include <LibWeb/CSS/BooleanExpression.h>
#include <LibWeb/CSS/CalculatedOr.h>
#include <LibWeb/CSS/MediaFeatureID.h>
#include <LibWeb/CSS/Parser/ComponentValue.h>
#include <LibWeb/CSS/Ratio.h>

namespace Web::CSS {

// https://www.w3.org/TR/mediaqueries-4/#typedef-mf-value
class MediaFeatureValue {
public:
    explicit MediaFeatureValue(Keyword ident)
        : m_value(move(ident))
    {
    }

    explicit MediaFeatureValue(LengthOrCalculated length)
        : m_value(move(length))
    {
    }

    explicit MediaFeatureValue(Ratio ratio)
        : m_value(move(ratio))
    {
    }

    explicit MediaFeatureValue(ResolutionOrCalculated resolution)
        : m_value(move(resolution))
    {
    }

    explicit MediaFeatureValue(IntegerOrCalculated integer)
        : m_value(move(integer))
    {
    }

    explicit MediaFeatureValue(i64 integer)
        : m_value(IntegerOrCalculated(integer))
    {
    }

    explicit MediaFeatureValue(Vector<Parser::ComponentValue> unknown_tokens)
        : m_value(move(unknown_tokens))
    {
    }

    String to_string() const;

    bool is_ident() const { return m_value.has<Keyword>(); }
    bool is_length() const { return m_value.has<LengthOrCalculated>(); }
    bool is_integer() const { return m_value.has<IntegerOrCalculated>(); }
    bool is_ratio() const { return m_value.has<Ratio>(); }
    bool is_resolution() const { return m_value.has<ResolutionOrCalculated>(); }
    bool is_unknown() const { return m_value.has<Vector<Parser::ComponentValue>>(); }
    bool is_same_type(MediaFeatureValue const& other) const;

    Keyword const& ident() const
    {
        VERIFY(is_ident());
        return m_value.get<Keyword>();
    }

    LengthOrCalculated const& length() const
    {
        VERIFY(is_length());
        return m_value.get<LengthOrCalculated>();
    }

    Ratio const& ratio() const
    {
        VERIFY(is_ratio());
        return m_value.get<Ratio>();
    }

    ResolutionOrCalculated const& resolution() const
    {
        VERIFY(is_resolution());
        return m_value.get<ResolutionOrCalculated>();
    }

    IntegerOrCalculated integer() const
    {
        VERIFY(is_integer());
        return m_value.get<IntegerOrCalculated>();
    }

private:
    Variant<Keyword, LengthOrCalculated, Ratio, ResolutionOrCalculated, IntegerOrCalculated, Vector<Parser::ComponentValue>> m_value;
};

// https://www.w3.org/TR/mediaqueries-4/#mq-features
class MediaFeature final : public BooleanExpression {
public:
    enum class Comparison : u8 {
        Equal,
        LessThan,
        LessThanOrEqual,
        GreaterThan,
        GreaterThanOrEqual,
    };

    // Corresponds to `<mf-boolean>` grammar
    static NonnullOwnPtr<MediaFeature> boolean(MediaFeatureID id)
    {
        return adopt_own(*new MediaFeature(Type::IsTrue, id));
    }

    // Corresponds to `<mf-plain>` grammar
    static NonnullOwnPtr<MediaFeature> plain(MediaFeatureID id, MediaFeatureValue value)
    {
        return adopt_own(*new MediaFeature(Type::ExactValue, move(id), move(value)));
    }
    static NonnullOwnPtr<MediaFeature> min(MediaFeatureID id, MediaFeatureValue value)
    {
        return adopt_own(*new MediaFeature(Type::MinValue, id, move(value)));
    }
    static NonnullOwnPtr<MediaFeature> max(MediaFeatureID id, MediaFeatureValue value)
    {
        return adopt_own(*new MediaFeature(Type::MaxValue, id, move(value)));
    }

    static NonnullOwnPtr<MediaFeature> half_range(MediaFeatureValue value, Comparison comparison, MediaFeatureID id)
    {
        return adopt_own(*new MediaFeature(Type::Range, id,
            Range {
                .left_value = move(value),
                .left_comparison = comparison,
            }));
    }
    static NonnullOwnPtr<MediaFeature> half_range(MediaFeatureID id, Comparison comparison, MediaFeatureValue value)
    {
        return adopt_own(*new MediaFeature(Type::Range, id,
            Range {
                .right_comparison = comparison,
                .right_value = move(value),
            }));
    }

    // Corresponds to `<mf-range>` grammar, with two comparisons
    static NonnullOwnPtr<MediaFeature> range(MediaFeatureValue left_value, Comparison left_comparison, MediaFeatureID id, Comparison right_comparison, MediaFeatureValue right_value)
    {
        return adopt_own(*new MediaFeature(Type::Range, id,
            Range {
                .left_value = move(left_value),
                .left_comparison = left_comparison,
                .right_comparison = right_comparison,
                .right_value = move(right_value),
            }));
    }

    virtual MatchResult evaluate(HTML::Window const*) const override;
    virtual String to_string() const override;
    virtual void dump(StringBuilder&, int indent_levels = 0) const override;

private:
    enum class Type : u8 {
        IsTrue,
        ExactValue,
        MinValue,
        MaxValue,
        Range,
    };

    struct Range {
        Optional<MediaFeatureValue> left_value {};
        Optional<Comparison> left_comparison {};
        Optional<Comparison> right_comparison {};
        Optional<MediaFeatureValue> right_value {};
    };

    MediaFeature(Type type, MediaFeatureID id, Variant<Empty, MediaFeatureValue, Range> value = {})
        : m_type(type)
        , m_id(move(id))
        , m_value(move(value))
    {
    }

    static MatchResult compare(HTML::Window const& window, MediaFeatureValue const& left, Comparison comparison, MediaFeatureValue const& right);
    MediaFeatureValue const& value() const { return m_value.get<MediaFeatureValue>(); }
    Range const& range() const { return m_value.get<Range>(); }

    Type m_type;
    MediaFeatureID m_id;
    Variant<Empty, MediaFeatureValue, Range> m_value {};
};

class MediaQuery : public RefCounted<MediaQuery> {
    friend class Parser::Parser;

public:
    ~MediaQuery() = default;

    // https://www.w3.org/TR/mediaqueries-4/#media-types
    enum class KnownMediaType : u8 {
        All,
        Print,
        Screen,
    };
    struct MediaType {
        FlyString name;
        Optional<KnownMediaType> known_type;
    };

    static NonnullRefPtr<MediaQuery> create_not_all();
    static NonnullRefPtr<MediaQuery> create() { return adopt_ref(*new MediaQuery); }

    bool matches() const { return m_matches; }
    bool evaluate(HTML::Window const&);
    String to_string() const;

private:
    MediaQuery() = default;

    // https://www.w3.org/TR/mediaqueries-4/#mq-not
    bool m_negated { false };
    MediaType m_media_type { .name = "all"_fly_string, .known_type = KnownMediaType::All };
    OwnPtr<BooleanExpression> m_media_condition { nullptr };

    // Cached value, updated by evaluate()
    bool m_matches { false };
};

String serialize_a_media_query_list(Vector<NonnullRefPtr<MediaQuery>> const&);

Optional<MediaQuery::KnownMediaType> media_type_from_string(StringView);
StringView to_string(MediaQuery::KnownMediaType);

}

namespace AK {

template<>
struct Formatter<Web::CSS::MediaFeature> : Formatter<StringView> {
    ErrorOr<void> format(FormatBuilder& builder, Web::CSS::MediaFeature const& media_feature)
    {
        return Formatter<StringView>::format(builder, media_feature.to_string());
    }
};

template<>
struct Formatter<Web::CSS::MediaQuery> : Formatter<StringView> {
    ErrorOr<void> format(FormatBuilder& builder, Web::CSS::MediaQuery const& media_query)
    {
        return Formatter<StringView>::format(builder, media_query.to_string());
    }
};

}
