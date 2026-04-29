from pypinyin import lazy_pinyin, Style, pinyin


def to_pinyin(text: str) -> str:
    if not text:
        return ""
    return "".join(lazy_pinyin(text)).lower()


def to_pinyin_abbr(text: str) -> str:
    if not text:
        return ""
    parts = pinyin(text, style=Style.FIRST_LETTER, strict=False)
    return "".join(item[0] for item in parts if item).lower()