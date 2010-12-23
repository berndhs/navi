CREATE TABLE "relationtags" (
  "relationid" TEXT NOT NULL,
  "key" TEXT NOT NULL,
  "value" TEXT NOT NULL,
  UNIQUE ("relationid","key") ON CONFLICT REPLACE
);
