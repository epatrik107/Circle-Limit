# Circle Limit Texture Rendering with Animated Star

This project is a C++ application that utilizes OpenGL for rendering. It combines the creation of a texture reminiscent of Circle Limit artworks by Maurits Cornelis Escher with the animation of a rotating/swirling four-pointed star. The virtual world follows Euclidean geometry, measuring distances in sea miles (tm). The world consists of a single four-pointed "star" located at the point (50, 30), with a length and height of 80 tm.

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes.

### Prerequisites

- A C++ compiler that supports C++11.
- OpenGL library installed on your system.

### Installing

1. Clone the repository to your local machine.
2. Open the project in your preferred C++ IDE (e.g., CLion).
3. Build the project using the IDE's build command.

## Usage

This project includes two main classes: `Texture` and `GPUProgram`.

- `Texture`: This class is responsible for loading, creating, and managing textures. It provides functionality for creating textures from files or from an image represented as a vector of `vec4`.

- `GPUProgram`: This class is responsible for creating, linking, and using GPU programs. It also provides methods to set uniform variables in the GPU program.

Users can interact with the program by adjusting the sharpness of the star with the 'h' key, toggling animation with the 'a' key, increasing/decreasing texture resolution with the 'r' and 'R' keys, and changing texture filtering mode with the 't' and 'T' keys.

## Contributing

Please read `CONTRIBUTING.md` for details on our code of conduct, and the process for submitting pull requests to us.

## License

This project is licensed under the MIT License - see the `LICENSE.md` file for details.

## Acknowledgments

- Thanks to the OpenGL community for the comprehensive documentation and tutorials.
- This project was inspired by the Circle Limit artworks of Maurits Cornelis Escher.
