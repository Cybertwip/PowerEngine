#version 330 core

in vec2 TexCoords;
layout(location = 0) out vec4 FragColor;
layout(location = 1) out int EntityID;
layout(location = 2) out int UniqueID;


uniform uint selectionColor;
uniform uint uniqueIdentifier;
uniform sampler2D texture1;

void main()
{
    FragColor = texture(texture1, TexCoords);
    EntityID = int(selectionColor);
    UniqueID = int(uniqueIdentifier);

}
