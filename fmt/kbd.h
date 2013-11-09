7050 // PC keyboard interface constants
7051 
7052 #define KBSTATP         0x64    // kbd controller status port(I)
7053 #define KBS_DIB         0x01    // kbd data in buffer
7054 #define KBDATAP         0x60    // kbd data port(I)
7055 
7056 #define NO              0
7057 
7058 #define SHIFT           (1<<0)
7059 #define CTL             (1<<1)
7060 #define ALT             (1<<2)
7061 
7062 #define CAPSLOCK        (1<<3)
7063 #define NUMLOCK         (1<<4)
7064 #define SCROLLLOCK      (1<<5)
7065 
7066 #define E0ESC           (1<<6)
7067 
7068 // Special keycodes
7069 #define KEY_HOME        0xE0
7070 #define KEY_END         0xE1
7071 #define KEY_UP          0xE2
7072 #define KEY_DN          0xE3
7073 #define KEY_LF          0xE4
7074 #define KEY_RT          0xE5
7075 #define KEY_PGUP        0xE6
7076 #define KEY_PGDN        0xE7
7077 #define KEY_INS         0xE8
7078 #define KEY_DEL         0xE9
7079 
7080 // C('A') == Control-A
7081 #define C(x) (x - '@')
7082 
7083 static uchar shiftcode[256] =
7084 {
7085   [0x1D] CTL,
7086   [0x2A] SHIFT,
7087   [0x36] SHIFT,
7088   [0x38] ALT,
7089   [0x9D] CTL,
7090   [0xB8] ALT
7091 };
7092 
7093 static uchar togglecode[256] =
7094 {
7095   [0x3A] CAPSLOCK,
7096   [0x45] NUMLOCK,
7097   [0x46] SCROLLLOCK
7098 };
7099 
7100 static uchar normalmap[256] =
7101 {
7102   NO,   0x1B, '1',  '2',  '3',  '4',  '5',  '6',  // 0x00
7103   '7',  '8',  '9',  '0',  '-',  '=',  '\b', '\t',
7104   'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',  // 0x10
7105   'o',  'p',  '[',  ']',  '\n', NO,   'a',  's',
7106   'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',  // 0x20
7107   '\'', '`',  NO,   '\\', 'z',  'x',  'c',  'v',
7108   'b',  'n',  'm',  ',',  '.',  '/',  NO,   '*',  // 0x30
7109   NO,   ' ',  NO,   NO,   NO,   NO,   NO,   NO,
7110   NO,   NO,   NO,   NO,   NO,   NO,   NO,   '7',  // 0x40
7111   '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
7112   '2',  '3',  '0',  '.',  NO,   NO,   NO,   NO,   // 0x50
7113   [0x9C] '\n',      // KP_Enter
7114   [0xB5] '/',       // KP_Div
7115   [0xC8] KEY_UP,    [0xD0] KEY_DN,
7116   [0xC9] KEY_PGUP,  [0xD1] KEY_PGDN,
7117   [0xCB] KEY_LF,    [0xCD] KEY_RT,
7118   [0x97] KEY_HOME,  [0xCF] KEY_END,
7119   [0xD2] KEY_INS,   [0xD3] KEY_DEL
7120 };
7121 
7122 static uchar shiftmap[256] =
7123 {
7124   NO,   033,  '!',  '@',  '#',  '$',  '%',  '^',  // 0x00
7125   '&',  '*',  '(',  ')',  '_',  '+',  '\b', '\t',
7126   'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',  // 0x10
7127   'O',  'P',  '{',  '}',  '\n', NO,   'A',  'S',
7128   'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',  // 0x20
7129   '"',  '~',  NO,   '|',  'Z',  'X',  'C',  'V',
7130   'B',  'N',  'M',  '<',  '>',  '?',  NO,   '*',  // 0x30
7131   NO,   ' ',  NO,   NO,   NO,   NO,   NO,   NO,
7132   NO,   NO,   NO,   NO,   NO,   NO,   NO,   '7',  // 0x40
7133   '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
7134   '2',  '3',  '0',  '.',  NO,   NO,   NO,   NO,   // 0x50
7135   [0x9C] '\n',      // KP_Enter
7136   [0xB5] '/',       // KP_Div
7137   [0xC8] KEY_UP,    [0xD0] KEY_DN,
7138   [0xC9] KEY_PGUP,  [0xD1] KEY_PGDN,
7139   [0xCB] KEY_LF,    [0xCD] KEY_RT,
7140   [0x97] KEY_HOME,  [0xCF] KEY_END,
7141   [0xD2] KEY_INS,   [0xD3] KEY_DEL
7142 };
7143 
7144 
7145 
7146 
7147 
7148 
7149 
7150 static uchar ctlmap[256] =
7151 {
7152   NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
7153   NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
7154   C('Q'),  C('W'),  C('E'),  C('R'),  C('T'),  C('Y'),  C('U'),  C('I'),
7155   C('O'),  C('P'),  NO,      NO,      '\r',    NO,      C('A'),  C('S'),
7156   C('D'),  C('F'),  C('G'),  C('H'),  C('J'),  C('K'),  C('L'),  NO,
7157   NO,      NO,      NO,      C('\\'), C('Z'),  C('X'),  C('C'),  C('V'),
7158   C('B'),  C('N'),  C('M'),  NO,      NO,      C('/'),  NO,      NO,
7159   [0x9C] '\r',      // KP_Enter
7160   [0xB5] C('/'),    // KP_Div
7161   [0xC8] KEY_UP,    [0xD0] KEY_DN,
7162   [0xC9] KEY_PGUP,  [0xD1] KEY_PGDN,
7163   [0xCB] KEY_LF,    [0xCD] KEY_RT,
7164   [0x97] KEY_HOME,  [0xCF] KEY_END,
7165   [0xD2] KEY_INS,   [0xD3] KEY_DEL
7166 };
7167 
7168 
7169 
7170 
7171 
7172 
7173 
7174 
7175 
7176 
7177 
7178 
7179 
7180 
7181 
7182 
7183 
7184 
7185 
7186 
7187 
7188 
7189 
7190 
7191 
7192 
7193 
7194 
7195 
7196 
7197 
7198 
7199 
