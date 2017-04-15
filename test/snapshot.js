'use strict'

import test from 'ava'
import ps from '../'

test('default', async t => {
  const tasks = await ps.snapshot()

  t.true(Array.isArray(tasks))
  t.not(tasks.length, 0)
  t.deepEqual(Object.keys(tasks[0]), ps.allowedFields)
})

test('one field', async t => {
  const tasks = await ps.snapshot('pid')

  t.true(Array.isArray(tasks))
  t.not(tasks.length, 0)
  t.deepEqual(Object.keys(tasks[0]), ['pid'])
})

test('multiple fields', async t => {
  const tasks = await ps.snapshot('pid', 'name')

  t.true(Array.isArray(tasks))
  t.not(tasks.length, 0)
  t.deepEqual(Object.keys(tasks[0]), ['name', 'pid'])
})
