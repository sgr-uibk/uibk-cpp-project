#!/usr/bin/env bash
set -euo pipefail

# Download PlantUML JAR if missing
if [[ ! -f plantuml-pdf.jar ]]; then
    echo "Downloading PlantUML-PDF..."
    curl -Lo plantuml-pdf.jar "https://github.com/plantuml/plantuml/releases/download/v1.2025.10/plantuml-pdf-1.2025.10.jar"
fi

if ! command -v clang-uml >/dev/null 2>&1; then
    echo "Error: 'clang-uml' command not found. Please install clang-uml, see https://clang-uml.github.io/md_docs_2installation.html"
    exit 1
fi

clang-uml -g plantuml
for i in *.puml; do
	java -jar plantuml-pdf.jar -tpdf $i;
done

echo "Done."
