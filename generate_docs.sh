#!/usr/bin/env bash
HERE="$(
  cd -- "$(dirname "$0")" >/dev/null 2>&1
  pwd -P
)"

set -e

PUSH_TO_S3=false
for arg in "$@"; do
  if [[ "$arg" == "--push-to-s3" ]]; then
    PUSH_TO_S3=true
  fi
done

echo "Script running from: $HERE"

DOCS_DIR="$HERE/docs"
PY_EXAMPLES_DIR="$DOCS_DIR/examples/python"
CPP_EXAMPLES_DIR="$DOCS_DIR/examples/cpp"

echo "Cleaning existing docs folder..."
rm -rf "$DOCS_DIR"
mkdir -p "$PY_EXAMPLES_DIR" "$CPP_EXAMPLES_DIR"

echo "Copying root README.md to index.md..."
cp "$HERE/README.md" "$DOCS_DIR/index.md"

# Copy image assets
mkdir -p "$DOCS_DIR/images"
cp -r "$HERE/images/"* "$DOCS_DIR/images/" || true

# Create LICENSE page
echo "# License" > "$DOCS_DIR/license.md"
echo "" >> "$DOCS_DIR/license.md"
cat "$HERE/LICENSE" >> "$DOCS_DIR/license.md"

echo "Copying Python example READMEs..."
find "$HERE/examples/python" -name README.md | while read -r f; do
  name=$(basename "$(dirname "$f")")
  cp "$f" "$PY_EXAMPLES_DIR/${name}.md"
done

echo "Copying C++ example READMEs..."
find "$HERE/examples/cpp" -name README.md | while read -r f; do
  name=$(basename "$(dirname "$f")")
  cp "$f" "$CPP_EXAMPLES_DIR/${name}.md"
done

# Fix the README links in the main index
sed -i 's|/README.md|.md|g' "$DOCS_DIR/index.md"
sed -i 's|LICENSE|license.md|g' "$DOCS_DIR/index.md"

# Copy the hide footer and logo click script
mkdir -p "$DOCS_DIR/css" "$DOCS_DIR/js"
cp "$HERE/main.css" "$DOCS_DIR/css/main.css"
cp "$HERE/logo-click.js" "$DOCS_DIR/js/logo-click.js"

echo "Running mkdocs build..."
mkdocs build --config-file "$HERE/mkdocs.yml"

echo "MkDocs site generated in $HERE/site/"

# Upload to S3
if $PUSH_TO_S3; then
  echo "Uploading to S3..."
  aws s3 sync "$HERE/site/" s3://zmq.nodarsensor.net --delete
fi
