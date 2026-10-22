/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file input_sha256_check.cpp Standalone probe for the actual native fixture hash helper.
 * @see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Groups9-10.
 * Compile output/intermediates and supplied vector files belong in test-cache/.
 */

#include "../../test_input_sha256.h"
#include <iostream>

/**
 * Print native file identities for independent known-vector/Python comparison.
 * @param argc One plus the number of explicitly selected fixture paths.
 * @param argv Program name followed by input paths; no outputs are created.
 * @return Zero after all hashes print, two for missing paths, one on read failure.
 */
int main(int argc, char **argv)
{
	if (argc < 2) return 2;
	try {
		// The harness supplies independent vectors, including padding boundaries.
		// Every argument is a read-only file; filename contents do not select data.
		for (int index = 1; index < argc; ++index) std::cout << TestInputSha256(argv[index]) << '\n';
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
	return 0;
}
