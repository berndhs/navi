CREATE TABLE "relationtags" (
  "relationid" INTEGER PRIMARY KEY NOT NULL,
  "key" TEXT NOT NULL,
  "value" TEXT NOT NULL,
  UNIQUE ("relationid","key") ON CONFLICT REPLACE
);
