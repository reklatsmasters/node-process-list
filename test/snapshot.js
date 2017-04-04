'use strict'

import test from 'ava'
import { snapshot } from '../'

test('should work', async t => {
  const tasks = await snapshot()

  t.true(Array.isArray(tasks))
  t.not(tasks.length, 0)
  t.is(typeof tasks[0], 'string')
})

test('should work verbose', async t => {
  const tasks = await snapshot({verbose: true})

  t.true(Array.isArray(tasks))
  t.not(tasks.length, 0)
  t.deepEqual(Object.keys(tasks[0]), ['name', 'pid', 'ppid', 'path', 'threads', 'owner', 'priority'])
})
