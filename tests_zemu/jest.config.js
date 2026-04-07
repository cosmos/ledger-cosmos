module.exports = {
  preset: 'ts-jest',
  testEnvironment: 'node',
  transformIgnorePatterns: ['node_modules/(?!(get-port)/)'],
  transform: {
    '^.+\\.tsx?$': ['ts-jest', { diagnostics: false }],
    '^.+\\.js$': ['ts-jest', { tsconfig: { allowJs: true } }],
  },
}
