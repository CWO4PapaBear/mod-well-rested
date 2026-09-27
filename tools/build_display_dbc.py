#!/usr/bin/env python3
"""Add original display-only WotLK Spell.dbc rows to an operator-supplied DBC.

No client data is distributed. Never replace a conflicting ID. Use the same
IDs, durations and bonus text on server and client; check world SQL separately.
"""
import argparse
import struct
from pathlib import Path

TEXT_FIELDS = list(range(136, 152)) + list(range(170, 186)) + list(range(187, 203))


def decode(data):
    magic, count, fields, size, strings = struct.unpack_from('<4s4I', data)
    if (magic, fields, size) != (b'WDBC', 234, 936) or len(data) != 20 + count * size + strings:
        raise ValueError('Expected complete WotLK 234-field Spell.dbc')
    rows = [list(r) for r in struct.iter_unpack('<234I', data[20:20 + count * size])]
    if len({r[0] for r in rows}) != count:
        raise ValueError('Duplicate spell IDs')
    return rows, data[20 + count * size:]


def definitions(rest_id=910100, reward_id=910101, rest_icon=44, reward_icon=117, bonus=8):
    if rest_id == reward_id or min(rest_id, reward_id, rest_icon, reward_icon, bonus) <= 0:
        raise ValueError('Distinct positive spell IDs, positive icons and bonus required')
    result = {}
    for id_, icon, duration in ((rest_id, rest_icon, 347), (reward_id, reward_icon, 367)):
        row = [0] * 234
        row[0] = id_
        row[4] = 0x80000000 | 0x00800000  # no cancel, allow dead
        row[5] = 0x20  # does not break stealth
        row[6] = 1  # allow dead target
        row[7] = 0x00100000  # aura survives death
        row[40] = duration
        row[46] = 13
        row[68] = 0xffffffff
        row[71] = 6  # APPLY_AURA
        row[74] = 1
        row[86] = 1  # caster
        row[95] = 4  # DUMMY, deliberately NOT MOD_XP_PCT
        row[133] = icon
        row[216:219] = [0x3f800000] * 3
        result[id_] = row
    texts = {
        rest_id: ('Resting', 'Remain alive and out of combat inside an inn until this timer ends to gain Well Rested. Each completed rest refreshes the reward and starts another rest timer. Leaving the inn or logging out resets unfinished rest.'),
        reward_id: ('Well Rested', f'Monster kills grant {bonus}% additional experience. Remaining time pauses while logged out. Completing another inn rest refreshes this reward; it does not stack with itself.'),
    }
    return result, texts


def transform(data, **kwargs):
    rows, strings = decode(data)
    new, texts = definitions(**kwargs)
    collisions = set(new) & {r[0] for r in rows}
    if collisions:
        raise ValueError('Spell-ID collision; reconcile before patching: ' + str(sorted(collisions)))
    for id_, row in new.items():
        name, description = texts[id_]
        for field, text in ((136, name), (170, description), (187, description)):
            row[field] = len(strings)
            strings += text.encode('utf8') + b'\0'
    result = rows + list(new.values())
    result.sort(key=lambda r: r[0])
    return struct.pack('<4s4I', b'WDBC', len(result), 234, 936, len(strings)) + b''.join(struct.pack('<234I', *r) for r in result) + strings


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path)
    parser.add_argument('output', type=Path)
    for key, default in [('rest-id', 910100), ('reward-id', 910101), ('rest-icon', 44), ('reward-icon', 117), ('bonus', 8)]:
        parser.add_argument('--' + key, type=int, default=default)
    args = parser.parse_args()
    if args.input.resolve() == args.output.resolve() or args.output.exists():
        parser.error('Use a new output path; never overwrite live data')
    args.output.write_bytes(transform(args.input.read_bytes(), **{k: getattr(args, k) for k in ('rest_id', 'reward_id', 'rest_icon', 'reward_icon', 'bonus')}))


if __name__ == '__main__':
    main()
