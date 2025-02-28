<?php

namespace PhpGit2\Backend;

class PHPSerializedConfigBackend extends \GitConfigBackend {
    private $level;
    private $storage = [];

    private $path;
    private $file;

    private $locked = false;
    private $lockedContent = '';

    private $iterator = false;

    public function open($level,$repo) {
        $repoPath = git_repository_path($repo);
        if (!$this->readonly && !empty($repoPath)) {
            $this->path = "${repoPath}config.phpserial";
            $this->level = $level;
            $this->file = fopen($this->path,'c+');
            $this->readIn();
        }
    }

    public function get($name) {
        if (!isset($this->storage[$name][0])) {
            return false;
        }

        return [
            'value' => $this->storage[$name][0],
            'name' => $name,
            'level' => $this->level,
        ];
    }

    public function set($name,$value) {
        $this->checkSnapshot();

        $this->storage[$name][0] = $value;
        $this->writeOut();
    }

    public function set_multivar($name,$regexp,$value) {
        $this->checkSnapshot();

        if (!isset($this->storage[$name])) {
            $this->storage[$name] = [];
        }

        $bucket =& $this->storage[$name];

        // NOTE: It seems the behavior in the default config file backend is to
        // append to the multivar if none of the values match.

        $n = 0;
        $found = false;
        while (isset($bucket[$n])) {
            if (preg_match($regexp,$bucket[$n])) {
                $found = true;
                $bucket[$n] = $value;
            }
            $n += 1;
        }

        if (!$found) {
            $bucket[] = $value;
        }

        $this->writeOut();
    }

    public function del($name) {
        $this->checkSnapshot();

        unset($this->storage[$name]);
        $this->writeOut();
    }

    public function del_multivar($name,$regexp) {
        $this->checkSnapshot();

        if (!isset($this->storage[$name])) {
            return;
        }

        $bucket =& $this->storage[$name];

        $n = 0;
        while ($n < count($bucket)) {
            if (preg_match($regexp,$bucket[$n])) {
                unset($bucket[$n]);
            }
            $n += 1;
        }

        if (count($bucket) > 0) {
            // Reorder the keys so it is an indexed array.
            $this->storage[$name] = array_values($bucket);
        }
        else {
            unset($this->storage[$name]);
        }

        $this->writeOut();
    }

    public function iterator_new() {
        $this->iterator = $this->storage;
    }

    public function iterator_next($context) {
        if ($this->iterator === false) {
            return false;
        }

        $value = current($this->iterator);
        next($this->iterator);

        return $value[0];
    }

    public function snapshot() {
        if ($this->readonly) {
            return $this;
        }
        $snapshot = clone $this;
        $snapshot->readonly = true;
        return $snapshot;
    }

    public function lock() {
        $this->locked = true;
    }

    public function unlock() {
        $this->locked = false;

        if ($this->lockedContent && is_resource($this->file)) {
            fseek($this->file,0);
            fwrite($this->file,$this->lockedContent);
            ftruncate($this->file,strlen($this->lockedContent));            
            $this->lockedContent = '';
        }
    }

    private function writeOut() : void {
        $content = serialize($this->storage);

        if ($this->locked) {
            $this->lockedContent = $content;
        }
        else if (is_resource($this->file)) {
            fseek($this->file,0);
            fwrite($this->file,$content);
            ftruncate($this->file,strlen($content));
        }
    }

    private function readIn() : void {
        $this->storage = [];
        if (is_resource($this->file)) {
            $contents = stream_get_contents($this->file);
            if (!empty($contents)) {
                $contents = unserialize($contents);
                if (is_array($contents)) {
                    $this->storage = $contents;
                }
            }
        }
    }

    private function checkSnapshot() : void {
        if ($this->readonly) {
            throw new \Git2Exception("cannot modify a snapshot config");
        }
    }
}
