#!/bin/bash

set -e

TESTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
[ -d "$TESTDIR" ] || { echo invalid test dir; exit 1; }

COMPRESS=${COMPRESS:-${TESTDIR}/lzw}
echo "## Using $COMPRESS in $TESTDIR"

function   compress_ { "${COMPRESS}" "$@"; }
function uncompress_ { "${COMPRESS}" -d "$@"; }

TMPDIR="$(mktemp -d)"
trap 'rm -rf "${TMPDIR}"' EXIT

[ -d "$TMPDIR" ] || { echo "invalid directory : $TMPDIR"; exit 1; }
cd "${TMPDIR}"

info bash -o - | shuf -n50 >file.txt

compress -c file.txt >file.txt.Z.2
[ -e file.txt.Z.2 ]

echo "## Test (de)compression"
compress_ file.txt file.txt.Z
[ -e file.txt.Z ]

diff file.txt.Z file.txt.Z.2

uncompress_ file.txt.Z file.txt.2
[ -e file.txt.2 ]
diff file.txt file.txt.2

echo "## Test (de)compression file to stdout"
compress_ file.txt >file.txt.Z
[ -e file.txt.Z ]

diff file.txt.Z file.txt.Z.2

uncompress_ file.txt.Z file.txt.2
[ -e file.txt.2 ]
diff file.txt file.txt.2

echo "## Test (de)compression stdin to stdout"
compress_ <file.txt >file.txt.Z
[ -e file.txt.Z ]

diff file.txt.Z file.txt.Z.2

uncompress_ file.txt.Z file.txt.2
[ -e file.txt.2 ]
diff file.txt file.txt.2

echo "## OK."
