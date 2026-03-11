-- Fix Retainer Owner CharacterId
--
-- Problem:
--   Some older builds stored `chararetainerinfo.CharacterId` as `charainfo.EntityId` (32-bit)
--   instead of the persistent `charainfo.CharacterId` (64-bit).
--
-- What this does:
--   Updates `chararetainerinfo.CharacterId` by joining on `charainfo.EntityId`.
--
-- Safety / idempotency:
--   - Only rows that currently match a `charainfo.EntityId` are updated.
--   - Rows already using the correct 64-bit CharacterId are left untouched.
--
-- Recommended:
--   - Take a DB backup first.
--   - Run this during server downtime.

START TRANSACTION;

-- 1) Preflight: how many rows look like legacy ownership?
SELECT
  COUNT(*) AS legacy_owner_rows
FROM chararetainerinfo r
JOIN charainfo c
  ON r.CharacterId = c.EntityId
WHERE r.CharacterId <> c.CharacterId;

-- 2) Optional: snapshot impacted rows (uncomment if you want a quick backup table)
-- CREATE TABLE IF NOT EXISTS chararetainerinfo__pre_fix_owner AS
--   SELECT r.*
--   FROM chararetainerinfo r
--   JOIN charainfo c ON r.CharacterId = c.EntityId
--   WHERE r.CharacterId <> c.CharacterId;

-- 3) Apply fix
UPDATE chararetainerinfo r
JOIN charainfo c
  ON r.CharacterId = c.EntityId
SET r.CharacterId = c.CharacterId
WHERE r.CharacterId <> c.CharacterId;

-- 4) Postflight: should now be zero
SELECT
  COUNT(*) AS legacy_owner_rows_after
FROM chararetainerinfo r
JOIN charainfo c
  ON r.CharacterId = c.EntityId
WHERE r.CharacterId <> c.CharacterId;

COMMIT;
