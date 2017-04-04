'use strict'

const ps = require('./')

ps.snapshot().then(tasks => console.log(tasks))
