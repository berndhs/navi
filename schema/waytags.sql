CREATE TABLE "waytags" (
  "wayid" TEXT NOT NULL,
  "key" TEXT NOT NULL,
  "value" TEXT NOT NULL,
  UNIQUE ("wayid","key") ON CONFLICT REPLACE
);
