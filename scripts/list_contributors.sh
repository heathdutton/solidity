#!/usr/bin/env bash

# ------------------------------------------------------------------------------
# Creates a list containing names of people who contributed to the project, for use
# in release notes. The names come from the author field on the commits between
# the current revision and the one specified as argument.
#
# Note that the output often requires extra manual processing to remove entries
# that refer to the same person (diacritics vs no diacritics, name vs nickname, etc.).
#
# Usage:
#    <script name>.sh <revision> [<target-ref>]
#
# Arguments:
#    <revision>    - Starting revision (e.g., v0.8.32)
#    <target-ref>  - Target ref to compare against (default: develop)
#                    Examples: develop, origin/develop, upstream/develop
#
# ------------------------------------------------------------------------------
# This file is part of solidity.
#
# solidity is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# solidity is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with solidity.  If not, see <http://www.gnu.org/licenses/>
#
# (c) 2023 solidity contributors.
#------------------------------------------------------------------------------

set -euo pipefail

script_dir=$(dirname "$0")
# shellcheck source=scripts/common.sh
source "${script_dir}/common.sh"

(( $# >= 1 && $# <= 2 )) || fail "Wrong number of arguments. Usage: $0 <revision> [<target-ref>]."

revision="$1"
# Default to local develop and its configured tracking branch
target_ref="${2:-develop}"

upstream=$(git rev-parse --abbrev-ref "${target_ref}@{upstream}" 2>/dev/null || true)
echo "Listing contributors from ${revision} to ${target_ref}${upstream:+ (tracking: $upstream)}..." >&2

# NOTE: Commas are removed from any names containing them. It would look confusing otherwise, given
# that the list is delimited by commas. Hopefully no contributor uses a comma as their nickname.
git shortlog --summary "${revision}..${target_ref}" |
    cut --field 2 |
    tr --delete , |
    sort |
    uniq |
    paste --serial --delimiter=, |
    sed -e 's/,/, /g'
