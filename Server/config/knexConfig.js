module.exports = {
  development: {
    client: 'postgresql',
    useNullAsDefault: true,
    connection: {
      host: '10.10.0.x',
      user: 'dummyName',
      password: 'dummyPassword'
      filename: './example.db'
    }
  },

  production: {
    client: 'postgresql',
    connection: {
      database: 'example'
    },
    pool: {
      min: 2,
      max: 10
    }
  }
};