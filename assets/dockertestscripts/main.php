<?php
if (function_exists('\php\retrieve_version_str')) {
    echo "version str: " . \php\retrieve_version_str() . "\n";
}

if (function_exists('\php\retrieve_major_version')) {
    echo "major version: " . \php\retrieve_major_version() . "\n";
}

if (function_exists('\php\retrieve_minor_version')) {
    echo "minor version: " . \php\retrieve_minor_version() . "\n";
}

if (function_exists('\php\retrieve_patch_version')) {
    echo "patch version: " . \php\retrieve_patch_version() . "\n";
}
