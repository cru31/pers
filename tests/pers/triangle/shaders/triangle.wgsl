// Simple triangle shader - vertex positions hardcoded
// No vertex buffer needed for first test

@vertex
fn vs_main(@builtin(vertex_index) vertexIndex: u32) -> @builtin(position) vec4<f32> {
    // Hardcoded triangle vertices
    var positions = array<vec2<f32>, 3>(
        vec2<f32>( 0.0,  0.5),  // Top
        vec2<f32>(-0.5, -0.5),  // Bottom left
        vec2<f32>( 0.5, -0.5)   // Bottom right
    );
    
    let position = positions[vertexIndex];
    return vec4<f32>(position, 0.0, 1.0);
}

@fragment
fn fs_main() -> @location(0) vec4<f32> {
    // Red color
    return vec4<f32>(1.0, 0.0, 0.0, 1.0);
}