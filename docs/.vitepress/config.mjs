import {defineConfig} from 'vitepress'

const repoURL = 'https://github.com/CauldronDevelopmentLLC/cbang'

export default defineConfig({
  srcExclude: ['node_modules/**', '**/README.md'],

  title:       'C! / cbang',
  description: 'A cross-platform C++ utility library.',
  lang:        'en-US',
  cleanUrls:   true,
  lastUpdated: true,

  head: [
    ['link', {rel: 'icon', href: '/favicon.ico'}],
    ['meta', {name: 'theme-color', content: '#8b0000'}],
  ],

  themeConfig: {
    logo:      '/cbang.png',
    siteTitle: 'C! / cbang',

    nav: [
      {text: 'Home',  link: '/'},
      {text: 'Docs',  link: '/Application'},
      {text: 'GitHub', link: repoURL},
    ],

    sidebar: [
      {
        text: 'Framework',
        items: [
          {text: 'Application',             link: '/Application'},
          {text: 'Configuration & Options', link: '/Config'},
          {text: 'Logging',                 link: '/Logging'},
          {text: 'Exceptions',              link: '/Exceptions'},
          {text: 'SmartPointer',            link: '/SmartPointer'},
          {text: 'Enumerations',            link: '/Enumerations'},
        ],
      },
      {
        text: 'Networking',
        items: [
          {text: 'HTTP/HTTPS/WebSocket', link: '/WebServer'},
          {text: 'API Framework',        link: '/API'},
          {text: 'Async DNS',            link: '/AsyncDNS'},
          {text: 'OpenSSL',              link: '/OpenSSL'},
          {text: 'ACMEv2',               link: '/ACMEv2'},
          {text: 'Net Primitives',       link: '/Net'},
          {text: 'WebSocket Lifecycle',  link: '/Websocket_Lifecycle'},
        ],
      },
      {
        text: 'Data',
        items: [
          {text: 'JSON',             link: '/JSON'},
          {text: 'SQLite',           link: '/SQLite'},
          {text: 'MariaDB',          link: '/MariaDB'},
          {text: 'String Utilities', link: '/String'},
        ],
      },
      {
        text: 'Runtime',
        items: [
          {text: 'Event System',     link: '/EventSystem'},
          {text: 'Subprocess & OS',  link: '/Subprocess'},
        ],
      },
      {
        text: 'Reference',
        items: [
          {text: 'Building V8 (Linux)',     link: '/Build_V8_Linux'},
          {text: 'Profiling with perf',     link: '/how_to_profile_with_linux_perf'},
        ],
      },
    ],

    socialLinks: [{icon: 'github', link: repoURL}],

    search: {provider: 'local'},

    editLink: {
      pattern: `${repoURL}/edit/master/docs/:path`,
      text:    'Edit on GitHub',
    },

    footer: {
      message:   'Released under the LGPL v2.1+ License.',
      copyright: '© 2003–2026 Cauldron Development.',
    },
  },
})
