
// -----------

const char* htmlCtrlAttribute = R"HTML(
    {{ name }}="{{ value }}"
)HTML";

// -----------

const char* htmlCtrlTag = R"HTML(
    <{{ tag }}{{ attributes }} />
)HTML";

// -----------

const char* htmlCtrlTagEmpty = R"HTML(
    <{{ tag }}{{ attributes }}></{{ tag }}>
)HTML";

// -----------

const char* htmlCtrlHeader = R"HTML(
    <h1 id="{{ id }}" class="{{ class }}">{{ text }}</h1>
)HTML";

// -----------

const char* htmlCtrlLabel = R"HTML(
    <label id="{{ id }}" name="{{ name }}" class="{{ class }}">{{ text }}</label>
)HTML";

// -----------

const char* htmlCtrlInput = R"HTML(
    <input id="{{ id }}" name="{{ name }}" class="{{ class }}" type="{{ type }}" value="{{ value }}" {{ checked }} placeholder="{{ placeholder }}" onchange="{{ onchange }}">
)HTML";

// -----------

const char* htmlCtrlSelect = R"HTML(
    <select id="{{ id }}" name="{{ name }}" class="{{ class }}" {{ multiple }} onchange="{{ onchange }}"></select>
)HTML";

// -----------

const char* htmlCtrlOption = R"HTML(
    <option id="{{ id }}" name="{{ name }}" class="{{ class }}" value="{{ value }}" {{ selected }}>{{ text }}</option>
)HTML";

// -----------

const char* htmlCtrlTextarea = R"HTML(
    <textarea id="{{ id }}" name="{{ name }}" class="{{ class }}" rows="{{ rows }}" cols="{{ cols }}" onchange="{{ onchange }}">{{ text }}</textarea>
)HTML";

// -----------

const char* htmlCtrlButton = R"HTML(
    <button id="{{ id }}" name="{{ name }}" class="{{ class }}" onclick="{{ onclick }}">{{ text }}</button>
)HTML";

    // todo: fieldset and legend
    // todo: datalist (autocomplete)
// -----------

const char* htmlCtrlOutput = R"HTML(
    <output id="{{ id }}" name="{{ name }}" class="{{ class }}">{{ text }}</output>
)HTML";

// -----------

    // todo: canvas (and draw)
