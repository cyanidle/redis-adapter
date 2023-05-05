#ifndef IMPL_SETTINGSCOMMENT_H
#define IMPL_SETTINGSCOMMENT_H

#define COMMENT(field, ...) \
private: void _priv_comment_test_ ## field () const noexcept { \
        _has_Is_Serializable(); \
        static_assert(sizeof(this->field) && sizeof(& THIS_TYPE ::_priv_getFinalPtr_##field), \
                      "Please use COMMENT() on existing fields created with FIELD()!"); \
} \
    QString _priv_comment_get_##field() const { \
    return QStringLiteral(__VA_ARGS__); \
} \
    Q_PROPERTY(QString __field_comment__ ##field READ _priv_comment_get_##field) \
    public:

#define CLASS_COMMENT(cls, ...) \
private: \
void _priv_comment_test_root () const noexcept {_has_Is_Serializable(); \
static_assert(::std::is_same_v<cls, THIS_TYPE>, "Please use the class itself as first argument");} \
QString _priv_comment_get_root() const {return QStringLiteral(__VA_ARGS__);} \
Q_PROPERTY(QString __root_comment__ ## cls READ _priv_comment_get_root) \
public:

#endif // IMPL_SETTINGSCOMMENT_H
