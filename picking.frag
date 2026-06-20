#version 410 core
uniform uint object_id;
out uint frag_id;

void main() {
    frag_id = object_id;
}